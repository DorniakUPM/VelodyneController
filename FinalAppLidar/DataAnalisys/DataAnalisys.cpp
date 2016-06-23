#include "DataAnalisys.h"
DataAnalisys::DataAnalisys(List<Punto3D^>^ puntosController, List<Obstaculo^>^ ObstaculosController, cli::array<Object^>^ ParamAnalisys, List<int>^ Conclusiones, cli::array<bool>^ Flags, cli::array<Thread^>^ Threads, OpenGl^ Dibujador)
{
	try {
		this->Informe = Informe;
		this->Threads = Threads;
		this->Flags = Flags;
		parametros = ParamAnalisys;
		parametros[INFORME_ANALISYS] = " ";
		this->Conclusiones = Conclusiones;
		matriz = puntosController;
		ObstaculosvAnt = ObstaculosController;
		Obstaculos = gcnew List<Obstaculo^>();
		Cercanos = gcnew cli::array<bool>(4);
		cambios = 0;
		PCercanos = gcnew cli::array<Punto3D^>(4);
		if (!thread_analysis) {
			thread_analysis = gcnew Thread(gcnew ThreadStart(this, &DataAnalisys::Esperar));
		}
		thread_analysis->Start();
		//Guarda el identificador de thread en el array de threads del Controllerador 
		Threads[1] = thread_analysis;
	}
	catch (Exception^ e) {
		Informar(e->ToString());
	}
}
//matriz::Lista de puntos
//resolucionAngular::resolucion del laser
//Vcoche::velocidad actual del coche
//consigna velocidad:: puntero para devolucion de parametro de velocidad
//consigna volante:: puntero para devolucion de parametro de volante
//apertura::Angulo de interes de lectura en grados
//List<Punto3D^>^ matriz, double resolucionAngular,double Vcoche, double &consigna_velocidad, double &consigna_volante, double apertura
void DataAnalisys::AnalisysThread()
{
	Informar("Iniciando Thread Analisis");
	while (Flags[FLAG_ANALISYS] && !Flags[FLAG_WARNING] && !Flags[FLAG_PAUSA])
	{
		Informar("Interior del while");
		try
		{
			if (!Flags[FLAG_TRATAMIENTO]) {
				Informar("Tratamiento");
				resolutionH = Convert::ToDouble(parametros[HORIZONTAL_RESOLUTION]);//Resolucion
				resolutionV = Convert::ToDouble(parametros[VERTICAL_RESOLUTION]);//Resolucion
				VCOCHE = Convert::ToDouble(parametros[CAR_VELOCITY]);//Vcoche
				apertura = Convert::ToDouble(parametros[OPENING]);//Apertura
				NUMERO_COLUMNAS = matriz->Count / NUMERO_FILAS;

				if (/*VCOCHE > 5*/true) {
					if (!comprobarBloqueo(matriz))
					{
						Segmentacion(matriz, apertura);
						//TODO::Identificar tipo de obstaculo
						if (Obstaculos->Count > 0) {
							EliminarObstaculos();//Elimina obstaculos no validos
							prepararObstaculos();//Geometria del obstaculo
							//RelacionarObstaculos();//Tracking

							copiarObstaculos();//Copiar el vector de obstaculos obtenido en Controller y comprueba colisiones
						}
					}
					else {
						Informar("Bloqueo");
						consigna_velocidad = 0.0;
					}
				}
				else {
					Informar("Error Velocidad minima tratamiento");
				}

				//Fin tratamiento
				//Actualizar consignas en el vector de conclusiones
				Conclusiones[0] = consigna_velocidad;
				Conclusiones[1] = consigna_volante;
			}
		}
		catch (Exception^ e)
		{
			Informar("Excepcion" + e->ToString());
			Flags[FLAG_WARNING] = true;
		}
	}
	//En caso de que se desactive y se reactive despues hay que limpiar los objetos
	ObstaculosvAnt->Clear();
	Esperar();
}

void DataAnalisys::Esperar()
{
	while (Flags[FLAG_WARNING] || Flags[FLAG_PAUSA]) {
		Sleep(200);
	}
	AnalisysThread();
}

void DataAnalisys::Informar(String ^ Entrada)
{
	parametros[INFORME_ANALISYS] += "[" + DateTime::Now.ToString("HH - mm - ss") + "]" + Entrada + "\r\n";
}

void DataAnalisys::copiarObstaculos()
{
	//Controller de collision
	if (Flags[FLAG_TRATAMIENTO]) {
		Flags[FLAG_WARNING] = true;
		Informar("Error de Colision");
	}
	Flags[FLAG_TRATAMIENTO] = true;

	ObstaculosvAnt->Clear();
	ObstaculosvAnt->AddRange(Obstaculos);

	if (Flags[FLAG_OPENGL]) {
		Informar("Obstaculos->OpenGl");
		Dibujador->modificarObstaculos(ObstaculosvAnt);
	}
	Obstaculos->Clear();
}

void DataAnalisys::Segmentacion(List<Punto3D^>^ matrix, double apertura)
{
	//int inicio = (NUMERO_COLUMNAS / 2); //- aprr;
	//int final = (NUMERO_COLUMNAS / 2); //+ aprr;
	int inicio = 0;
	int final = NUMERO_COLUMNAS;

	//Se recorre la matriz linealmente
	Informar("Comienzo del recorrido de la matriz");
	for (int fila = 0; fila <= NUMERO_FILAS - 1; fila++)//Recorrido de filas
	{
		for (int columna = inicio; columna < final; columna++)//Recorrido de columnas
		{

			//Se comprubea si el punto a tratar Existe
			if (matrix[convaPos(columna, fila)]->valido && (matrix[convaPos(columna, fila)]->getAzimuth()>(180 - apertura)) && (matrix[convaPos(columna, fila)]->getAzimuth() < (180 + apertura)))
			{
				ResetParametros();
				//En caso de que sea el primer punto se asigna directamente al obstaculo 0
				if (columna == 0 && fila == inicio)
				{
					//Mete al final del vector de obstaculos un obstaculo que crea
					Obstaculos->Add(gcnew Obstaculo());
					matrix[convaPos(columna, fila)]->setObstacle(Obstaculos->Count - 1);
					//Accede al obstaculo 0, al vector de componentes, mente el punto en el vector de componentes
					Obstaculos[Obstaculos->Count - 1]->components->Add(matrix[convaPos(columna, fila)]);
				}
				else {
					//Se compara cada punto a tratar con sus puntos adyacentes ya tratados

					if (fila > 0)
					{
						//Punto de encima misma columna fila-1
						if (puntosCercanosV(matrix[convaPos(columna, fila)], matrix[convaPos(columna, fila - 1)]))
						{
							Cercanos[0] = true;
							PCercanos[0] = matrix[convaPos(columna, fila - 1)];
						}
						if (columna > 0)
						{
							//Punto de encima a la izquierda columna-1 fila-1
							if (puntosCercanosD(matrix[convaPos(columna, fila)], matrix[convaPos(columna - 1, fila - 1)]))
							{
								Cercanos[1] = true;
								PCercanos[1] = matrix[convaPos(columna - 1, fila - 1)];
							}
						}
						if (columna < NUMERO_COLUMNAS - 1)
						{
							//Punto de encima a la derecha columna+1 fila-1
							if (puntosCercanosD(matrix[convaPos(columna, fila)], matrix[convaPos(columna + 1, fila - 1)]))
							{
								Cercanos[2] = true;
								PCercanos[2] = matrix[convaPos(columna + 1, fila - 1)];
							}
						}
					}
					if (columna > 0)
					{
						//Punto de la izquierda columna-1 fila
						if (puntosCercanosH(matrix[convaPos(columna, fila)], matrix[convaPos(columna - 1, fila)]))
						{
							Cercanos[3] = true;
							PCercanos[3] = matrix[convaPos(columna - 1, fila)];
						}
					}

					numCercanos = 0;
					for (int recorrido = 0; recorrido < 4; recorrido++)
					{
						if (Cercanos[recorrido])
						{
							numCercanos++;
							localizador = recorrido;
						}
					}

					if (numCercanos == 0)
					{
						Obstaculos->Add(gcnew Obstaculo());
						matrix[convaPos(columna, fila)]->setObstacle(Obstaculos->Count - 1);
						Obstaculos[Obstaculos->Count - 1]->components->Add(matrix[convaPos(columna, fila)]);
					}
					else if (numCercanos == 1) {
						matrix[convaPos(columna, fila)]->setObstacle(PCercanos[localizador]->getObstacle());
						Obstaculos[matrix[convaPos(columna, fila)]->getObstacle()]->components->Add(matrix[convaPos(columna, fila)]);
					}
					else {
						for (int recorrido2 = 0; recorrido2 < 4; recorrido2++)
						{
							if (Cercanos[recorrido2]) {
								if (Obstmenor == -1) {
									Obstmenor = PCercanos[recorrido2]->getObstacle();
								}
								else if (PCercanos[recorrido2]->getObstacle() != Obstmenor) {
									iguales = false;
									if (PCercanos[recorrido2]->getObstacle() < Obstmenor)
									{
										Obstmenor = PCercanos[recorrido2]->getObstacle();
									}
								}
							}
						}
						if (iguales) {
							matrix[convaPos(columna, fila)]->setObstacle(Obstmenor);
							Obstaculos[Obstmenor]->components->Add(matrix[convaPos(columna, fila)]);
						}
						else {
							for (int recorrido3 = 0; recorrido3 < 4; recorrido3++) {
								if (Cercanos[recorrido3]) {
									if (PCercanos[recorrido3]->getObstacle() != Obstmenor) {
										MoverObstaculo(PCercanos[recorrido3]->getObstacle(), Obstmenor);
										cambios++;
									}
								}
							}
							matrix[convaPos(columna, fila)]->setObstacle(Obstmenor);
							Obstaculos[Obstmenor]->components->Add(matrix[convaPos(columna, fila)]);
						}
					}
				}
			}
		}
	}
}

void DataAnalisys::ResetParametros() {
	Obstmenor = -1;
	for (int i = 0; i < 4; i++)
	{
		Cercanos[i] = false;
		PCercanos[i] = nullptr;
	}

	iguales = true;
}

void DataAnalisys::prepararObstaculos()
{
	for (int i = 0; i < Obstaculos->Count; i++)
	{
		Obstaculos[i]->prepareObstacle();//TODO::Calcular centro,cubo,y todo lo necesario
	}
}

void DataAnalisys::EliminarObstaculos()
{
	int ObstPeque�os = 0;
	int ObstEliminados = 0;
	for (int p = 0; p < Obstaculos->Count; p++)
	{
		if (!Obstaculos[p]->Valido) {
			Obstaculos->RemoveAt(p);
			p--;
			ObstEliminados++;
		}
		else if (Obstaculos[p]->components->Count < TAMA�O_MINIMO_OBSTACULO)
		{
			//TODO::Ajustar el numero minimo de puntos para considerarlo un obstaculo
			Obstaculos->RemoveAt(p);
			p--;
			ObstPeque�os++;
		}
	}
}

void DataAnalisys::RelacionarObstaculos()
{
	//TODO::Ajustar variables para este laser
	for (int i = 0; i < Obstaculos->Count; i++)
	{
		indice = -1;
		for (int j = 0; j < ObstaculosvAnt->Count; j++)
		{
			if (ObstaculosvAnt[j]->getVelocity() >= 1)
			{
				//0.3 es la distancia maxima que que puede recorrer entre barridos
				if (Obstaculos[i]->getCenter()->distanceToPoint(ObstaculosvAnt[j]->getPrediceCenter()) < (VCOCHE / 3.6) * TIEMPO_MARGEN)// POrque 0.3??  = 0.0833
				{
					relacionarVel(i, j);
					indice = j;
				}
			}
			else if (Obstaculos[i]->getCenter()->distanceToPoint(ObstaculosvAnt[j]->getCenter()) < DISTANCIA_MAXIMA /*&& fabs(Obstaculos[i].getYaw() - Obstaculos[i].getYaw()) < 5*/)
			{
				relacionarPos(i, j, VCOCHE, resolutionH);
				indice = j;
			}
		}
	}
}

void DataAnalisys::relacionarVel(int i, int j)
{
	Obstaculos[i]->setDirection(ObstaculosvAnt[j]->getCenter());
	Obstaculos[i]->calculatePrediceCenter();
	Obstaculos[i]->setVelocity(VCOCHE, resolutionH);
	Obstaculos[i]->calculateTimeToCollision(VCOCHE);
}

void DataAnalisys::relacionarPos(int i, int j, int VelC, int Res)
{
	Obstaculos[i]->setDirection(ObstaculosvAnt[j]->getCenter());
	Obstaculos[i]->calculatePrediceCenter();
	Obstaculos[i]->setVelocity(VCOCHE, resolutionH);
}

bool DataAnalisys::comprobarBloqueo(List<Punto3D^>^ matriz)
{

	//Devuelve true cuando hay bloqueo
	int medio = (matriz->Count / 16) / 2;
	Punto3D^ prueba;

	for (int k = medio - 30; k < medio + 30; k++) {
		for (int i = 0; i < NUMERO_FILAS; i++) {
			if (matriz[convaPos(k, i)]->valido && matriz[convaPos(k, i)]->getDistance() < 15 && matriz[convaPos(k, i)]->getCoordinatesZ() > ALTURA_MINIMA_OBST) {//Altura de bloquepmayor k 0.2
				prueba = matriz[convaPos(k, i)];
				return true;
			}
		}
	}
	return false;
}

bool DataAnalisys::puntosCercanosH(Punto3D^ p1, Punto3D^ p2)
{
	double tolerancia = p1->getDistance() * tan(resolutionV * PI / 180);
	tolerancia = tolerancia * ((100 + HORIZONTAL_TOLERANCE) / 100);
	return(tolerancia > p1->distanceToPoint(p2));
}

bool DataAnalisys::puntosCercanosV(Punto3D^ p1, Punto3D^ p2)
{
	double toleranciaV = p1->getDistance() * tan(resolutionV  * PI / 180);
	double toleranciaH = p1->getDistance() * tan(((resolutionH - (2 * 20 * 0.00001843 * 180)) / 16)  * PI / 180);//Cambiar la frecuencia
	Punto3D ^ a = gcnew Punto3D(toleranciaH, toleranciaV, 0);
	double tolerancia = a->getModule();
	//double tolerancia = p1->getDistance() * tan(resolutionV  * PI / 180);
	tolerancia = tolerancia * ((100 + VERTICAL_TOLERANCE) / 100);
	return(tolerancia > p1->distanceToPoint(p2));
}

bool DataAnalisys::puntosCercanosD(Punto3D^ p1, Punto3D^ p2)
{
	double toleranciaV = p1->getDistance() * tan(resolutionV  * PI / 180);
	double toleranciaH = p1->getDistance() * tan(resolutionH  * PI / 180);
	Punto3D ^a = gcnew Punto3D(toleranciaH, toleranciaV, 0);
	double tolerancia = a->getModule();
	tolerancia = tolerancia * ((100 + DIAGONAL_TOLERANCE) / 100);
	return(tolerancia > p1->distanceToPoint(p2));
}

int DataAnalisys::convaPos(int columna, int fila) {

	switch (fila)
	{
	case 1: fila = 2;
	case 2: fila = 4;
	case 3: fila = 6;
	case 4: fila = 8;
	case 5: fila = 10;
	case 6: fila = 12;
	case 7: fila = 14;
	case 8: fila = 1;
	case 9: fila = 3;
	case 10: fila = 5;
	case 11: fila = 7;
	case 12: fila = 9;
	case 13: fila = 11;
	case 14: fila = 13;
	default: break;
	}
	return columna * 16 + fila;
}

void DataAnalisys::MoverObstaculo(int Obst1, int Obst2)
{
	for (int i = 0; i < Obstaculos[Obst1]->components->Count; i++) {
		Obstaculos[Obst1]->components[i]->setObstacle(Obst2);
		Obstaculos[Obst2]->components->Add(Obstaculos[Obst1]->components[i]);
	}
	Obstaculos[Obst1]->Valido = false;
}
