#pragma once

enum t_examen{B, S, D};
enum t_resultado{p, n, r}; //r = - (Repetir)

struct examen{
  int id;
  t_examen tipo;
  t_resultado resultado;
  int quantity;
  examen() {}
  examen(int id, t_examen tipo, int q);
};

struct memS{
  int i;
  int ie;
  int q;
  int oe;
  int s;
  int b;
  int d;

  int examId;

  //refs
  int scout;

  int buffsEntrada;
  int mutexEntrada;
  int llenosEntrada;
  int vaciosEntrada;
  int inEntrada;
  int outEntrada;

  int buffsInternos;
  int mutexInternos;
  int llenosInternos;
  int vaciosInternos;
  int inInternos;
  int outInternos;

  int buffsSalida;
  int mutexSalida;
  int llenosSalida;
  int vaciosSalida;
  int inSalida;
  int outSalida;
};
