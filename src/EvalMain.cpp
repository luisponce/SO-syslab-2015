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

map<string, int> initArgs;

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
    // initValues["-i"] = value;
    cout << "Colas input: " << value << endl;
  } else if(mode == "-ie"){
    cout << "Capacidad input: " << value << endl;
  } else if(mode == "-oe"){
    cout << "Capacidad output: " << value << endl;
  } else if(mode == "-b"){
    cout << "Reactivos sangre: " << value << endl;
  } else if(mode == "-d"){
    cout << "Reactivos detritos: " << value << endl;
  } else if(mode == "-s"){
    cout << "Reactivos piel: " << value << endl;
  } else if(mode == "-q"){
    cout << "Capacidad colas internas: " << value << endl;
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

/*
/  Obtiene un puntero a la posicion de la memoria compartida
/  indicadas entre los bytes offset y len
*/
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

void SetInitialValues(){
  void *startSem = GetMem(0, sizeof(sem_t));
  sem_t *mutexMem = (sem_t *) startSem;
  sem_init(mutexMem, 0, 1);
  
  

}

void SetDefaultValues(){
  initArgs.insert(pair<string, int>("-i", 5)); 
  initArgs.insert(pair<string, int>("-ie", 6));
  initArgs.insert(pair<string, int>("-oe", 10));
  initArgs.insert(pair<string, int>("-b", 100));
  initArgs.insert(pair<string, int>("-d", 100));
  initArgs.insert(pair<string, int>("-s", 100));
  initArgs.insert(pair<string, int>("-q"), 10);
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

  CreateSharedMem();
  
  SetInitialValues();
  
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
