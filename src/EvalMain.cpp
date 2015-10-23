/*
/ Evaluator Main
/
/
*/

// -----PENDIENTES ---------------------------
//Crear metodo que valida cadena de caracteres
//Crear metodo que valide cadena de numeros

#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <map>

using namespace std;

string memName = "evaluator";

map<char, int> initArgs;

void PrintArgs(){
  cout << "colas input: " << initArgs['i'] << endl;
  cout << "Capacidad input: " << initArgs['I'] << endl;
  cout << "Capacidad output: " << initArgs['O'] << endl;
  cout << "Reactivos sangre: " << initArgs['b'] << endl;
  cout << "Reactivos detritos: " << initArgs['d'] << endl;
  cout << "Reactivos piel: " << initArgs['s'] << endl;
  cout << "Capacidad colas internas: " << initArgs['q'] << endl;
}

/*
/	mapea un par de argumentos, dado un modo y su valor(int)
/ 	Modos:
/	-i 		Colas de input
/	-ie     	Capacidad cola input
/	-oe     	Capacidad cola output
/	-b 		Reactivos sangre
/	-d 		Reactivos detritos
/	-s 		Reactivos piel
/       -q		Capacidad colas internas
/
/	NOTA: No procesa el argumento -n, este debe ser procesado
/ 	antes...
*/
void MapArg(string mode, int value){
  if(mode == "-i"){
    initArgs['i'] = value;
  } else if(mode == "-ie"){
    initArgs['I'] = value;
  } else if(mode == "-oe"){
    initArgs['O'] = value;
  } else if(mode == "-b"){
    initArgs['b'] = value;
  } else if(mode == "-d"){
    initArgs['d'] = value;
  } else if(mode == "-s"){
    initArgs['s'] = value;
  } else if(mode == "-q"){
    initArgs['q'] = value;
  } else {
    cout << "Usage: Invalid Argument" << endl;
    return;
  }

  
}

int CalculateMemMaxSize(){
	int size = 0;
 	//Memoria "estatica"
	size += 1 * sizeof(sem_t); //mutex memoria estatica
	size += 4 * sizeof(int); //informacion de las colas
	size += 3 * sizeof(int); //reactivos
	size += 12 * sizeof(int); //refs memoria dinamica
	//Memoria "Dinamica"
	

	return size;
}

void CreateSharedMem(){
	//crear memoria
	int shmfd;
	const char * memRegName = memName.c_str();
  	shmfd = shm_open(memRegName, O_RDWR | O_CREAT | O_TRUNC,
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

/*
/  Obtiene un puntero a la posicion de la memoria compartida
/  indicadas entre los bytes offset y len
*/
void* GetMem(int offset, int len){
	int shmfd;
  	shmfd = shm_open(memName.c_str(), O_RDWR , 0660);

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

/*
/ Obtiene una copia de la memoria compartida
/ 
/ i    Valor
/ ------------   
/ 1    -i
/ 2    -ie
/ 3    -q
/ 4    -oe
/ 5    React s
/ 6    React B
/ 7    React D
/ 9-12 Refs a MemD
/ 
*/
int GetIntFromMemS(int i){
  sem_t *mutex = (sem_t*) GetMem(0, sizeof(sem_t));
  while(sem_trywait(mutex) != 0);

  int offset = sizeof(sem_t);
  offset += sizeof(int)*i;
  int* val = (int *) GetMem(offset, sizeof(int));
  int res = *val;

  sem_post(mutex);
  
  return res;

}

void SetInitialValues(){
  int off = 0;

  sem_t *mutexMem = (sem_t *) GetMem(off, sizeof(sem_t));
  off += sizeof(sem_t);
  sem_init(mutexMem, 0, 1);
  
  int *numEntradas = (int *) GetMem(off, sizeof(int));
  off += sizeof(int);
  *numEntradas = initArgs['i'];

}

void SetDefaultValues(){
  initArgs.insert(pair<char, int>('i', 5)); //-i
  initArgs.insert(pair<char, int>('I', 6));//-ie
  initArgs.insert(pair<char, int>('O', 10));//-oe
  initArgs.insert(pair<char, int>('b', 100));
  initArgs.insert(pair<char, int>('d', 100));
  initArgs.insert(pair<char, int>('s', 100));
  initArgs.insert(pair<char, int>('q', 10));
}

void Initialize(int argc, string argv[]){
  cout<<"initialize "<<endl;
	
  //mapear argumentos
  int curArg = 0;
  if((argc % 2) == 0) {
    while(curArg+1 < argc){
      if(argv[curArg] == "-n"){
	memName = argv[curArg+1];
      } else {
	MapArg(argv[curArg], stoi(argv[curArg+1]));
      }
      curArg += 2;
    }
  } else {
    cout << "Usage: Invalid Argument" << endl;
    return;
  }
  SetDefaultValues();
  PrintArgs();
  
  CreateSharedMem();
  
  SetInitialValues();
  
  cout<< "-i from memS: " << GetIntFromMemS(0) << endl;
  
  delete [] argv;
  return;
}

void MapArgControl(string mode){
  if(mode == "list"){
    cout << "List of all system: " << mode << endl;
  } else if(mode == "list waiting"){
	  cout << "list of waiting elements: " << mode << endl;
  } else if(mode == "list processing"){
    cout << "List of processing elements: " << mode << endl;
  } else if(mode == "list reported"){
    cout << "List of reported elements: " << mode << endl;
  } else if(mode == "list reactive"){
    cout << "List of reactive: " << mode << endl;
  } else if(mode == "list all"){
    cout << "List of all system: " << mode << endl;
  } else if(mode.substr(0,9) == "update B "){
    cout << "Updating B reactives: " << mode.substr(0,9) << "with "
	 << mode.substr(9)<< endl; //Metodo convertir a numero y lanzar error en caso que no se pueda convertir
  } else if(mode.substr(0,9) == "update S "){
    cout << "Updating S reactives: " << mode.substr(0,9) << "with "
	 << mode.substr(9)<< endl; //Metodo convertir a numero y lanzar error en caso que no se pueda convertir
  } else if(mode.substr(0,9) == "update D "){
    cout << "Updating D reactives: " << mode.substr(0,9) << "with "
	 << mode.substr(9)<< endl; //Metodo convertir a numero y lanzar error en caso que no se pueda convertir
  } else {
    cout << "Usage: Invalid Argument" << endl;
    return;
  }
}

void SubControl() {
	string arg = "";
	while (!getline (cin,arg).eof()) {
    MapArgControl(arg);
	}
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

	//mapear argumentos
	int curArg = 0;
	if(argc == 0) {
		cout << "Nombre seccion compartida: " << "Por default" << endl;
		SubControl();
	} else if(argc == 2){
		if(argv[curArg] == "-s"){
			cout << "Nombre seccion compartida: " << argv[curArg+1] << endl;
			SubControl();
		} else {
			cout << "Usage: Invalid Argument" << endl;
			return;
		}
	} else {
		cout << "Usage: Invalid Argument" << endl;
		return;
	}

	delete [] argv;
	//SubControl();
	return;
}

void MapArgRep(string mode, int value){
	if(mode == "-i"){
		cout << "Modo interactivo: " << value << endl;
	} else if(mode == "-m"){
		cout << "Numero determinado examenes: " << value << endl;
	} else {
		cout << "Usage: Invalid Argument" << endl;
		return;
	}
}

void Report(int argc, string argv[]){
	cout << "report" << endl;

	//mapear argumentos
	int curArg = 0;
	if(argc == 0) {
		cout << "Usage: Invalid Argument" << endl;
		return;
	}
	if((argc % 2) == 0) {
		while(curArg+1 < argc){
			if(argv[curArg] == "-s"){
				if(argc == 2) {
					cout << "Usage: Invalid Argument" << endl;
					return;
				}
				cout << "Nombre seccion compartida: " << argv[curArg+1] << endl;
			} else {
				MapArgRep(argv[curArg], stoi(argv[curArg+1]));
			}
			curArg += 2;
		}
	} else {
		cout << "Usage: Invalid Argument" << endl;
		return;
	}

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
