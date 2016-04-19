#include "Controller.h"
#include "Parametros.h"
Controller::Controller()
{
	Obstaculos = gcnew List<Obstaculo^>();
	Puntos = gcnew List<Punto3D^>();
	Conclusiones = gcnew List<int>();
	Flags = gcnew cli::array<bool>(SIZE_ARRAY_FLAGS);
	for (int a = 0; a < SIZE_ARRAY_FLAGS; a++) {
		Flags[a] = false;
	}
	Flags[FLAG_PAUSA] = true;
	Flags[FLAG_TRATAMIENTO] = true;
	ArrayDataAnalisys = gcnew cli::array<Object^>(SIZE_ARRAY_ANALISYS);
	ArrayDataReader = gcnew cli::array<Object^>(SIZE_ARRAY_READER);
	ArrayGps = gcnew cli::array<Object^>(SIZE_ARRAY_GPS);
	Threads = gcnew cli::array<Thread^>(NUMERO_THREADS);
	for (int i = 0; i < SIZE_ARRAY_ANALISYS - 1; i++)
	{
		ArrayDataAnalisys[i] = 0;
	}
}

//TODO: INPLEMENTAR LA INTERPRETACIÓN DE CONCLUSIONES.
