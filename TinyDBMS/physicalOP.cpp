#include "physicalOP.h"


physicalOP * physicalOP::physicalop=NULL;


void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, int memory_block_index, Tuple& tuple) {
  Block* block_ptr;
  if (relation_ptr->getNumOfBlocks()==0) {
    cout << "The relation is empty" << endl;
    cout << "Get the handle to the memory block " << memory_block_index << " and clear it" << endl;
    block_ptr=mem.getBlock(memory_block_index);
    block_ptr->clear(); //clear the block
    block_ptr->appendTuple(tuple); // append the tuple
    cout << "Write to the first block of the relation" << endl;
    relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index);
  } else {
    cout << "Read the last block of the relation into memory block :" << endl;
    relation_ptr->getBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index);
    block_ptr=mem.getBlock(memory_block_index);

    if (block_ptr->isFull()) {
      cout << "(The block is full: Clear the memory block and append the tuple)" << endl;
      block_ptr->clear(); //clear the block
      block_ptr->appendTuple(tuple); // append the tuple
      cout << "Write to a new block at the end of the relation" << endl;
      relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index); //write back to the relation
    } else {
      cout << "(The block is not full: Append it directly)" << endl;
      block_ptr->appendTuple(tuple); // append the tuple
      cout << "Write to the last block of the relation" << endl;
      relation_ptr->setBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index); //write back to the relation
    }
  }  
}





void physicalOP::displayMem()
{
	cout << "The memory contains " << physicalop->mem.getMemorySize() << " blocks" << endl;
	cout << physicalop->mem << endl << endl;
}

void physicalOP::displaySchema(Schema &schema)
{
	cout << schema << endl;
  cout << "The schema has " << schema.getNumOfFields() << " fields" << endl;
  cout << "The schema allows " << schema.getTuplesPerBlock() << " tuples per block" << endl;
  cout << "The schema has field names: " << endl;
  vector<string> field_names=schema.getFieldNames();
  copy(field_names.begin(),field_names.end(),ostream_iterator<string,char>(cout," "));
  cout << endl;
  cout << "The schema has field types: " << endl;
  vector <enum FIELD_TYPE> field_types=schema.getFieldTypes();
  for (int i=0;i<schema.getNumOfFields();i++) {
    cout << (field_types[i]==0?"INT":"STR20") << "\t";
  }
  cout << endl;  
}

void physicalOP::displayRelation(string relation_name)
{
	Relation* relation_ptr=schema_manager.getRelation(relation_name);
	cout << "The table has name " << relation_ptr->getRelationName() << endl;
  cout << "The table currently have " << relation_ptr->getNumOfBlocks() << " blocks" << endl;
  cout << "The table currently have " << relation_ptr->getNumOfTuples() << " tuples" << endl ;
  cout << "Now the relation contains: " << endl;
  cout << *relation_ptr << endl<<endl;
}




Relation * physicalOP::CreateTable(string relation_name,
							 vector<string> & field_names, 
							 vector <enum FIELD_TYPE> & field_types)
{
	cout << "Creating a schema" << endl;
	Schema schema(field_names,field_types);
	//displaySchema(schema);
	cout << "Creating table " << relation_name << endl;
	Relation* relation_ptr=schema_manager.createRelation(relation_name,schema);
	displayRelation(relation_name);
	return relation_ptr;

}

void physicalOP::DropTable(string relation_name)
{
	cout << "Drop table " << relation_name << endl;
	schema_manager.deleteRelation(relation_name);
}

void physicalOP::insert(string relation_name,
				vector<string> field_names,
				vector<string> STRv,
				vector<int> INTv)
{
	cout<<"Insert a tuple to the relation "<<relation_name<<endl;
	Relation* relation_ptr = schema_manager.getRelation(relation_name);
	if (relation_ptr==NULL){
                cout << "No Such Relation"<<endl;
                return;
	}
	Tuple tuple = relation_ptr->createTuple();
	int counti=0;
	int counts=0;
	for(int i =0;i<tuple.getNumOfFields();i++)
	{
		if(tuple.getSchema().getFieldType(field_names[i])==INT)
		{
			tuple.setField(field_names[i],INTv[counti]);
			counti++;
		}
		else
		{
			tuple.setField(field_names[i],STRv[counts]);
			counts++;
		}
	}
	appendTupleToRelation(relation_ptr,mem,0,tuple);
	cout<<"Insert Done."<<endl;
	//cout<<*relation_ptr << endl << endl;
}

vector<Tuple> physicalOP::singleTableSelect(string relation_name,
						   condition con)
{
	cout<<"select"<<endl;
	vector<Tuple> result;
	Relation* relation_ptr = schema_manager.getRelation(relation_name);
	Block* block_ptr;
	if (relation_ptr==NULL){
                cout << "No Such Relation"<<endl;
                return result;
	}
	Tuple tuple = relation_ptr->createTuple();

	for(int i=0;i<relation_ptr->getNumOfBlocks();i++)
	{
		relation_ptr->getBlock(i,0);
		block_ptr=mem.getBlock(0);
		int NumOfTuples = block_ptr->getNumTuples();
		vector<Tuple> Tuples = block_ptr->getTuples();
		for(vector<Tuple>::iterator it  = Tuples.begin(); it != Tuples.end(); )  
        {  
			if(con.judge(*it))           //if this tuple fit the condition
			{
				result.push_back(*it);
				it = Tuples.erase(it);
			}
        }  
	}
	return result;
}


void physicalOP::Delete(string relation_name,              //using mem 0 to get the block, 1 to store the tuples to write back
						condition con)
{
	Relation* relation_ptr = schema_manager.getRelation(relation_name);
	Block* block_ptr,*block_back;
	int tuplesBack = 0;
	int blockback=0;
	int blockin;
	cout<<"Delete"<<endl;
	block_ptr=mem.getBlock(0);
	block_back=mem.getBlock(1);


	if (relation_ptr==NULL){
                cout << "No Such Relation"<<endl;
				return;
	}
	Tuple tuple = relation_ptr->createTuple();

	for(int i=0;i<relation_ptr->getNumOfBlocks();i++)
	{
		relation_ptr->getBlock(i,0);
		
		int NumOfTuples = block_ptr->getNumTuples();
		
		for(int j=0;j<NumOfTuples;j++)
		{
			if(!con.judge(block_ptr->getTuple(j)))
			{
				block_back->setTuple(tuplesBack,block_ptr->getTuple(j));
				tuplesBack++;
				if(tuplesBack==relation_ptr->getSchema().getTuplesPerBlock())
				{
					relation_ptr->setBlock(blockback,1);
					tuplesBack=0;
					blockback++;
				}
			}
		}
	}
	relation_ptr->deleteBlocks(blockback);


	return;
}


