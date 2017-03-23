/*
 * Symbol table implementation
 *
 */

#include <map>
#include <vector>
#include <iostream>
#include <string.h>
#include <utility>
#include "errors.h"

#include "symtable.h"
#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"

using namespace std;
// needs a global scope, either here or befor checks
// oonstructor
ScopedTable::ScopedTable(){}

// destructor 
ScopedTable::~ScopedTable() {  
	for (map<const char *, Symbol>::iterator it = symbols.begin(); it != symbols.end(); ++it)
		delete it->first;
} 

void ScopedTable::insert(Symbol &sym){

	char* name = sym.name;
	symbols.insert(std::pair<char*, Symbol>(name, sym));

}

void ScopedTable::remove(Symbol &sym){

	char* name = sym.name;
	symbols.erase(name);

}

// if it does not find it will return null
// and it seems okay to return null here
// handle null in Symbol table find
Symbol* ScopedTable::find(const char *name) {
	//cout <<"finding in scope ... "<<endl;
	SymbolIterator it = symbols.find(name);
	
	if (it == symbols.end()) {
		//cout<< "iterator reached end... "<<endl;
		return NULL;
	}
	else {
		//cout << "found in map... "<<"returning "<< it->second.name<<endl;
		return &it->second;
	}
}

// constructor
SymbolTable::SymbolTable(){

	returnType = NULL;
	noReturnFlag = false;
	funcFlag = false;
	breakFlag = false;
	
	this->push();
	//global scope
	//ScopedTable *globalScope = new ScopedTable();
	//tables.push_back(globalScope);
	
}

// destructor 
SymbolTable::~SymbolTable() {  

	while (!tables.empty()){
		tables.pop_back();
	}
} 

void SymbolTable::push(){
	//cout << "\nSymbolTable - push" << endl;
	
	ScopedTable* scope = new ScopedTable();
	tables.push_back(scope);
	
	//cout << "\nSymbolTable - push success" << endl;
	
}

void SymbolTable::pop(){
	if (!tables.empty()){
		tables.pop_back();
	}
}

void SymbolTable::insert(Symbol &sym){
	//cout << "size of symboltable "<<tables.size()<<endl;
	ScopedTable *st = tables.back();
	st->insert(sym);
}

void SymbolTable::remove(Symbol &sym){
	int size = tables.size() - 1;
	
	if (size >=0 ){
		tables[size]->remove(sym);
	}
}

Symbol* SymbolTable::find(const char *name) {
	//Symbol* symbol;
	int currentScope = tables.size() - 1;

	
		for (int i  = currentScope; i >= 0; i--){
			//cout<<"currentscope index is = "<< currentScope << " going to scopedtable find.."<<endl;
			Symbol* temp = tables[i]->find(name);
			if (temp != NULL)
				return temp;
		}
	
	return NULL;
}

bool MyStack::insideLoop(){
	if (!stmtStack.empty() ){//&& stmtStack.front() == <dynamic_cast>Type::){///&& stmtStack.top() == <dynamic_cast>){
		
		return true;
	}
	return false;
}

bool MyStack::insideSwitch(){

	if (!stmtStack.empty()){
		//stmtStack.pop_back();
		
		return true;
	}
	
	return false;
}
