/*
/ Evaluator Main
/
/
*/

#include <iostream>
#include <string>

using namespace std;

/* 	
/	dado un comando llama el metodo apropiado pasando 
/	el resto de argumentos MENOS el argumento que este
/	parser ya proceso.
/	ej: $ evaluator init -i 3
/	evalua -> evaluator init
/	pasa   -> -i 3
/
/	Comandos:
/	init 	initialize(argc, argv)
/ 	reg 	register(argc, argv)
/	ctrl	control(argc, argv)
/	rep		report(argc, argv)
*/
void CommandParser(int argc, char const *argv[]){
	if(argc > 1){
		string cmd = argv[1];
		if(cmd == "init"){
			cout<<"initialize"<<endl;
			return;
		} else if(cmd == "reg"){
			cout<<"register"<<endl;
			return;
		} else if(cmd == "ctrl"){
			cout<<"control"<<endl;
			return;
		} else if(cmd == "rep"){
			cout<<"report"<<endl;
			return;
		} 
	}

	cout<<"Error: Invalid Command"<<endl;
		return;
}

int main(int argc, char const *argv[])
{
	CommandParser(argc, argv);

	return 0;
}

