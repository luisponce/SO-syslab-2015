/*
/ Evaluator Main
/
/
*/
#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>

using namespace std;

string memName = "evaluator";

/*
/	mapea un par de argumentos, dado un modo y su valor(int)
/ 	Modos:
/	-i 		Colas de input
/	-ie 	Capacidad cola input
/	-oe 	Capacidad cola output
/	-b 		Reactivos sangre
/	-d 		Reactivos detritos
/	-s 		Reactivos piel
/
/	NOTA: No procesa el argumento -n, este debe ser procesado
/ 	antes...
*/
void MapArg(string mode, int value){
	if(mode == "-i"){
		cout<<"colas input: "<<value;
	} else if(mode == "-ie"){
		cout<<"Capacidad input: "<<value;
	}
}

int CalculateMemMaxSize(){
	int size = 0;
 	//Mutex De Memoria Compartida 
	size += 1 * sizeof(sem_t);

	return size;
}

void CreateSharedMem(){
	//crear memoria
	int shmfd;
	const char * memRegName = memName.c_str();
  	shmfd = shm_open("evaluator", O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 
  		0660);

  	if (shmfd < 0) {
	    perror("shm_open");
	    exit(1);
	}

	//Calcular tamaño total de la memoria compartida
	int memSize = CalculateMemMaxSize();

  	if (ftruncate(shmfd, memSize) < 0) {
	    perror("ftruncate");
	    exit(1);
  	}

  	close(shmfd);
}

void* GetMem(int offset, int len){
	int shmfd;
  	shmfd = shm_open("evaluator", O_RDWR , 0660);

  	if (shmfd < 0) {
	    perror("shm_open");
	    exit(1);
	}

	void* startshm;
  	if ((startshm = mmap(NULL, len, PROT_READ | PROT_WRITE, 
  		MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
		perror("mmap");
	    exit(1);
	}

	return startshm;
}

void Initialize(int argc, string argv[]){
	cout<<"initialize "<<endl;

	//mapear argumentos
	int curArg = 0;
	while(curArg+1 < argc){
		if(argv[curArg] == "-n"){
			memName = argv[curArg+1];
		} else {
			MapArg(argv[curArg], stoi(argv[curArg+1]));
		}

		curArg += 2;
	}

	CreateSharedMem();

	//initial values
	void *startSem = GetMem(0, sizeof(sem_t));
	sem_t *mutexMem = (sem_t *) startSem;
	sem_init(mutexMem, 0, 1);

	delete [] argv;
	return;
}

void Register(int argc, string argv[]){
	cout << "register" <<endl;

	void *startSem = GetMem(0, sizeof(sem_t));
	sem_t *mutexMem = (sem_t *) startSem;
	cout<<"waiting mutex"<<endl;
	while(sem_trywait(mutexMem)!=0);
	cout<<"IN mutex, press any key to exit"<<endl;
	char d;
	cin>>d;
	sem_post(mutexMem);
	cout<<"OUT mutex"<<endl;

	delete [] argv;
	return;
}

void Control(int argc, string argv[]){
	cout << "control" << endl;

	delete [] argv;
	return;
}

void Report(int argc, string argv[]){
	cout << "report" << endl;

	delete [] argv;
	return;
}

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

		//eliminar los argumentos ya procesados
		string *args = new string[argc-2];
		for(int i=2; i<argc; i++){
			args[i-2] = argv[i];
		}
		argc -= 2;


		if(cmd == "init"){
			Initialize(argc, args);
			return;
		} else if(cmd == "reg"){
			Register(argc, args);
			return;
		} else if(cmd == "ctrl"){
			Control(argc, args);
			return;
		} else if(cmd == "rep"){
			Report(argc, args);
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

