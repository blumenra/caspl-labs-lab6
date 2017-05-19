#include "variable.h"



variable* addVariable(variable** vars_set, char* name, char* value){


	variable* newVar = createdNewVar(name, value);

	variable* tmp = *vars_set;

	/*
		if the set is empty
	*/
	if(tmp == NULL){
		*vars_set = newVar;
		return newVar;
	}
	
	variable* nextTmpVar = tmp->next;

	/*
		if name equals to the name of the FIRST var in the set
	*/
	if(strcmp(tmp->name, newVar->name) == 0){
		*vars_set = newVar;
		newVar->next = nextTmpVar;

		freeVariable(tmp);

		return newVar;
	}
	while(tmp->next != NULL){
		
		nextTmpVar = tmp->next;
		
		/*
			if name equals to the name of the next var in the set
		*/
		if(strcmp(nextTmpVar->name, newVar->name) == 0){

			tmp->next = newVar;
			newVar->next = nextTmpVar->next;

			freeVariable(nextTmpVar);

			return newVar;
		}

		tmp = tmp->next;
	}

	/*
		there was no var with the same name- put the new var at the end of the set
	*/
	tmp->next = newVar;


	return newVar;

}


void freeVariableSet(variable** vars_set){

	while(*vars_set != NULL){
		variable* tmp = *vars_set;
		*vars_set = (*vars_set) -> next;
		freeVariable(tmp);
	}
}


void freeVariable(variable* var_to_remove){

	
	if(var_to_remove != NULL){
		free(var_to_remove->name);
		free(var_to_remove->value);
		free(var_to_remove);
	}
}

int removeVar(variable** vars_set, char* var_to_remove){
	
	if (*vars_set == NULL)
		return 0;

	variable* tmp_var = *vars_set;

	if (strcmp(tmp_var->name, var_to_remove) == 0){
		*vars_set = tmp_var->next;
		freeVariable(tmp_var);
		return 1;
	}
		
	while(tmp_var->next != NULL){
		
		if(strcmp(tmp_var->next->name, var_to_remove) == 0){

			variable* variable_to_remove = tmp_var->next;

			tmp_var->next = tmp_var->next->next;
			freeVariable(variable_to_remove);

			return 1;
		}

		tmp_var = tmp_var->next;
	}

	return 0;
}

void printVars(variable** vars_set){

	variable* tmp = *vars_set;
	if(tmp == NULL){
		printf("There are no environment variables.\n");
	}
	else{
		
		printf("Environment variables:\n");
		while(tmp != NULL){

			printVar(tmp);
			printf("\n");
			tmp = tmp->next;
		}
	}
}


void printVar(variable* var){

	if(var == NULL){
		fprintf(stderr, "The variable is not initialized!\n");
	}
	else{

		printf("name: %s\n", var->name);
		printf("value: %s\n", var->value);
	}
}


variable* createdNewVar(char* name, char* value){

	variable* newVar = (variable*) malloc(sizeof(variable));

	newVar->name = (char*) (malloc((sizeof(char)*(strlen(name))) + 1));
	memcpy(newVar->name, name, (strlen(name)));
	newVar->name[(strlen(name))] = '\0';
	

	newVar->value = (char*) (malloc((sizeof(char)*(strlen(value))) + 1));
	memcpy(newVar->value, value, (strlen(value)));
	newVar->value[(strlen(value))] = '\0';

	newVar->next = NULL;

	return newVar;
}

char* findVar(variable** vars, char* varName){

	variable* tmp = *vars;
	char* newValue = NULL;

	while(tmp != NULL){

		if(strcmp(tmp->name, varName) == 0){

			newValue = (char*) (malloc((sizeof(char)*(strlen(tmp->value))) + 1));
			memcpy(newValue, tmp->value, (strlen(tmp->value)));
			newValue[(strlen(tmp->value))] = '\0';

			break;
		}

		tmp = tmp->next;
	}

	return newValue;
}