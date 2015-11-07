/*
/ Evaluator Main
/
/
*/

// -----PENDIENTES ---------------------------
//Crear metodo que valida cadena de caracteres
//Crear metodo que valide cadena de numeros

#include "dataStructures.h"

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
#include <fstream>
#include <ostream>
#include <sstream>
#include <thread>

using namespace std;

string memName = "evaluator";

map<char, int> initArgs;


examen::examen(int id, t_examen tipo, int q){
  this->id = id;
  this->tipo = tipo;
  this->quantity = q;
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
	size += sizeof(memS);
	//Memoria "Dinamica"
	int in_num = initArgs['i'];
	int in_size = initArgs['I'];
	size += (sizeof(examen)*in_size)*in_num;
	size += sizeof(sem_t)*in_num*3; //mutex, llenos y vacios
	size += sizeof(int)*in_num*2; //in, out

	int q = initArgs['q'];
	size += sizeof(examen)*q*3;
	size += sizeof(sem_t)*3*3; //3 tipos de semaforos, 3 buffers
	size += sizeof(int)*3*2; //in, out

	int out = initArgs['O'];
	size += sizeof(examen) * out;
	size += sizeof(sem_t)*3;
	size += sizeof(int)*2;//in, out

  //variables auxiliares
  size += sizeof(sem_t);//scout

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

	close(shmfd);
	startshm += offset;
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
memS GetMemS(){
  sem_t *mutex = (sem_t*) GetMem(0, sizeof(sem_t));
  sem_wait(mutex);

  memS* val = (memS *) GetMem(sizeof(sem_t), sizeof(memS));
  memS res = *val;

  sem_post(mutex);

  return res;

}

void PrintArgs(){

  memS mem = GetMemS();
  cout<< "Colas input -i: "<< initArgs['i'] << " - from memS: " << mem.i  << endl;
  cout<< "Capacidad input -ie: "<< initArgs['I'] << " - from memS: " << mem.ie  << endl;
  cout<< "Capacidad colas internas -q: "<< initArgs['q'] << " - from memS: " << mem.q  << endl;
  cout<< "Capacidad output -oe: "<< initArgs['O'] << " - from memS: " << mem.oe  << endl;
  cout<< "Reactivos sangre -b: "<< initArgs['b'] << " - from memS: " << mem.b  << endl;
  cout<< "Reactivos detritos -d: "<< initArgs['d'] << " - from memS: " << mem.d  << endl;
  cout<< "Reactivos piel -s: "<< initArgs['s'] << " - from memS: " << mem.s  << endl;

}

/*
  initialize an array of sem_t with the given value,
  the given length, and in the given offset of the shared memory
 */
void InitSemArray(int *off, const int val, const int len ){
  for(int i = 0; i<len; i++){
    sem_t *sem = (sem_t *) GetMem(*off, sizeof(sem_t));
    *off += sizeof(sem_t);
    sem_init(sem, 1, val);
  }
}

/*
  initialize an array of int with the given value,
  the given length, and in the given offset of the shared memory
 */
void InitIntArray(int *off, const int val, const int len ){
  for(int i = 0; i<len; i++){
    int *num = (int *) GetMem(*off, sizeof(int));
    *off += sizeof(int);
    *num = val;
  }
}

void SetInitialValues(){
  int off = 0;

  //static mem sem
  sem_t *mutexMem = (sem_t *) GetMem(off, sizeof(sem_t));
  off += sizeof(sem_t);
  sem_init(mutexMem, 1, 1);

  int in_num = initArgs['i'];
  int in_size = initArgs['I'];
  int q = initArgs['q'];
  int out = initArgs['O'];

  //static structure
  memS *shmS = (memS *) GetMem(off, sizeof(memS));
  off += sizeof(memS);
  shmS->i = in_num;
  shmS->ie = in_size;
  shmS->q = q;
  shmS->oe = out;
  shmS->s = initArgs['s'];
  shmS->b = initArgs['b'];
  shmS->d = initArgs['d'];

  shmS->examId = 0;

  //dinamic mem
  //scout sem para cout
  shmS->scout = off;
  InitSemArray(&off, 1, 1);

  //input
  //buffers de entrada
  shmS->buffsEntrada = off;
  off += sizeof(examen) * in_size * in_num;
  //mutex de entradas
  shmS->mutexEntrada = off;
  InitSemArray(&off, 1, in_num);
  //Llenos Entradas
  shmS->llenosEntrada = off;
  InitSemArray(&off, 0, in_num);
  //vacios Entradas
  shmS->vaciosEntrada = off;
  InitSemArray(&off, in_size, in_num);
  //in Entrada
  shmS->inEntrada = off;
  InitIntArray(&off, 0, in_num);
  //out Entrada
  shmS->outEntrada = off;
  InitIntArray(&off, 0, in_num);

  //internas
  //buffers internos
  shmS->buffsInternos = off;
  off += sizeof(examen) * q * 3;
  //mutex internos
  shmS->mutexInternos = off;
  InitSemArray(&off, 1, 3);
  //Llenos Internos
  shmS->llenosInternos = off;
  InitSemArray(&off, 0, 3);
  //vacios Internos
  shmS->vaciosInternos = off;
  InitSemArray(&off, q, 3);
  //in Entrada
  shmS->inInternos = off;
  InitIntArray(&off, 0, 3);
  //out Entrada
  shmS->outInternos = off;
  InitIntArray(&off, 0, 3);

  //salida
  //buffer salida
  shmS->buffsSalida = off;
  off += sizeof(examen) * out;
  //mutex salida
  shmS->mutexSalida = off;
  InitSemArray(&off, 1, 1);
  //llenos salida
  shmS->llenosSalida = off;
  InitSemArray(&off, 0, 1);
  //vacios salida
  shmS->vaciosSalida = off;
  InitSemArray(&off, out, 1);
  //in Entrada
  shmS->inSalida = off;
  InitIntArray(&off, 0, 1);
  //out Entrada
  shmS->outSalida = off;
  InitIntArray(&off, 0, 1);
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

void EnqueueThread(int tray){
  for(;;){
    examen element;

    memS shms = GetMemS();
    sem_t *vacios = (sem_t*) 
      GetMem(shms.vaciosEntrada + (sizeof(sem_t)*tray), sizeof(sem_t));
    sem_t *mutex = (sem_t *)
      GetMem(shms.mutexEntrada + (sizeof(sem_t)*tray), sizeof(sem_t));
    sem_t *llenos = (sem_t *) 
      GetMem(shms.llenosEntrada + (sizeof(sem_t)*tray), sizeof(sem_t));
    sem_t *scout = (sem_t *) GetMem(shms.scout, sizeof(sem_t));

    sem_wait(llenos);
    sem_wait(mutex);

    int *out = (int *) 
      GetMem(shms.outEntrada + (sizeof(int)*tray), sizeof(int));
    examen *e = (examen *) 
      GetMem(shms.buffsEntrada + (sizeof(examen)*shms.ie*tray) 
       + (sizeof(examen) * *out), sizeof(examen));

    element = *e;
    *out = (*out + 1) % shms.ie;

    sem_post(mutex);
    sem_post(vacios);

    sem_wait(scout);
    cout << "EnQueue " << tray << ": Id Elemento recibido: " << element.id << endl;
    sem_post(scout);

    //decidir que cola va
    int destQueue;
    switch(element.tipo){
    case B:
      destQueue = 0;
      break;
    case S:
      destQueue = 1;
      break;
    case D:
      destQueue = 2;
      break;
    }

    vacios = (sem_t*) 
      GetMem(shms.vaciosInternos + (sizeof(sem_t)*destQueue), sizeof(sem_t));
    mutex = (sem_t *)
      GetMem(shms.mutexInternos + (sizeof(sem_t)*destQueue), sizeof(sem_t));
    llenos = (sem_t *) 
      GetMem(shms.llenosInternos + (sizeof(sem_t)*destQueue), sizeof(sem_t));
    
    sem_wait(vacios);
    sem_wait(mutex);

    int *in = (int *) 
      GetMem(shms.inInternos + (sizeof(int)*destQueue), sizeof(int));
    e = (examen *) 
      GetMem(shms.buffsInternos + (sizeof(examen)*shms.q*destQueue) 
	     + (sizeof(examen) * *in), sizeof(examen));
    
    *e = element; 
    *in = (*in + 1) % shms.q;
    
    sem_post(mutex);
    sem_post(llenos);

    sem_wait(scout);
    cout << "Enqueue " << tray;
    cout<< ": elemento " << element.id << " ingresado a cola interna " << destQueue << endl;
    sem_post(scout);

  }
}

void EvalThread(int tray){
  for(;;){
    examen element;

    memS shms = GetMemS();
    sem_t *vacios = (sem_t*) 
      GetMem(shms.vaciosInternos + (sizeof(sem_t)*tray), sizeof(sem_t));
    sem_t *mutex = (sem_t *)
      GetMem(shms.mutexInternos + (sizeof(sem_t)*tray), sizeof(sem_t));
    sem_t *llenos = (sem_t *) 
      GetMem(shms.llenosInternos + (sizeof(sem_t)*tray), sizeof(sem_t));
    sem_t *scout = (sem_t *) GetMem(shms.scout, sizeof(sem_t));

    sem_wait(llenos);
    sem_wait(mutex);

    int *out = (int *) 
      GetMem(shms.outInternos + (sizeof(int)*tray), sizeof(int));
    examen *e = (examen *) 
      GetMem(shms.buffsInternos + (sizeof(examen)*shms.q*tray) 
       + (sizeof(examen) * *out), sizeof(examen));

    element = *e;
    *out = (*out + 1) % shms.q;

    sem_post(mutex);
    sem_post(vacios);

    sem_wait(scout);
    cout << "Eval " << tray << ": Id Elemento recibido: " << element.id << endl;
    sem_post(scout);

    //TODO: proces exam

    vacios = (sem_t*) 
      GetMem(shms.vaciosSalida, sizeof(sem_t));
    mutex = (sem_t *)
      GetMem(shms.mutexSalida, sizeof(sem_t));
    llenos = (sem_t *) 
      GetMem(shms.llenosSalida, sizeof(sem_t));
    
    sem_wait(vacios);
    sem_wait(mutex);

    int *in = (int *) 
      GetMem(shms.inSalida, sizeof(int));
    e = (examen *) 
      GetMem(shms.buffsSalida + (sizeof(examen) * *in), sizeof(examen));
    
    *e = element; 
    *in = (*in + 1) % shms.oe;
    
    sem_post(mutex);
    sem_post(llenos);

    sem_wait(scout);
    cout << "Eval " << tray;
    cout<< ": elemento " << element.id << " ingresado a cola de salida " << endl;
    sem_post(scout);

  }
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

  PrintArgs();

  //hilos de colas de entrada
  for(int i = 0; i<initArgs['i']; i++){
    thread cur (EnqueueThread, i);
    cur.detach();
  }
    
    
  // hilos analizadores
  for(int i = 0; i<3; i++){
    thread cur (EvalThread, i);
    cur.detach();
  }

  delete [] argv;

  for(;;);

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

int GenSampleId(){
  int id;
  
  sem_t *mutex = (sem_t *) GetMem(0, sizeof(sem_t));
  sem_wait(mutex);

  memS *mem = (memS *) GetMem(sizeof(sem_t), sizeof(sem_t));
  id = mem->examId;
  mem->examId++;

  sem_post(mutex);
  
  return id;
}

void ProcesInput(istream& fs, ostream& out = cout){
  int tray, quantity;
  char type;
  string line;
  
  //proces multiple input lines
  while(getline(fs, line)){
    stringstream *ss = new stringstream(line);
    if(*ss>>tray && *ss>>type && *ss>>quantity){
      bool skip = false;
      t_examen tipo;
      switch(type){
        case 'B':
        	tipo = B;
        	break;
        case 'S':
        	tipo = S;
        	break;
        case 'D':
        	tipo = D;
        	break;
        default:
    	   skip = true;
    	   break;
      }

      int maxTray = GetMemS().i;
      if(tray >= maxTray) skip = true;
      if(quantity > 5) skip = true;
      
      if(!skip){
      	//cout<<"tray: "<<tray<<" type: "<<type<<" quantity: "<<quantity<<endl;
      	int id = GenSampleId();
	
      	examen *ex = new examen(id, tipo, quantity);
      	
      	memS shms = GetMemS();
      	sem_t *vacios = (sem_t*) 
      	  GetMem(shms.vaciosEntrada + (sizeof(sem_t)*tray), sizeof(sem_t));
      	sem_t *mutex = (sem_t *)
      	  GetMem(shms.mutexEntrada + (sizeof(sem_t)*tray), sizeof(sem_t));
      	sem_t *llenos = (sem_t *) 
      	  GetMem(shms.llenosEntrada + (sizeof(sem_t)*tray), sizeof(sem_t));
	
      	sem_wait(vacios);
      	sem_wait(mutex);

      	// ingresar elemento
      	int *in = (int *) 
      	  GetMem(shms.inEntrada + (sizeof(int)*tray), sizeof(int));
      	examen *e = (examen *) 
      	  GetMem(shms.buffsEntrada + (sizeof(examen)*shms.ie*tray) 
      		 + (sizeof(examen) * *in), sizeof(examen));

      	*e = *ex; 
      	*in = (*in + 1) % shms.ie;

      	sem_post(mutex);
      	sem_post(llenos);

      	//cout<<"id : "<<id<<endl;
	out<<id<<" ";
      } else {
        //cout<<"skiped!"<<endl;
      }
    }
    out<<endl;
    delete ss;
  }
}

void Register(int argc, string argv[]){
	cout << "register" <<endl;
	
	int curArg = 0;
	if(argc > 2 && argv[curArg] == "-n"){
	  //TODO: si argv no es string -> error
	  memName = argv[curArg+1];
	  curArg += 2; 
	}

	if(argc-curArg == 1 && argv[curArg]=="-"){
	  //modo interactivo
	  cout<<"Interactive mode:"<<endl;
	  ProcesInput(cin); 
	} else {
	  //leer de archivo
	  while(argc > curArg){
	    string fileName = argv[curArg];
	    fstream file;
	    ofstream output;
	    string outName = fileName.substr(0,fileName.length()-4) + ".spl";
	    file.open(fileName);
	    output.open(outName);
	    ProcesInput(file, output);
	    file.close();
	    curArg++;
	  }
	}

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
