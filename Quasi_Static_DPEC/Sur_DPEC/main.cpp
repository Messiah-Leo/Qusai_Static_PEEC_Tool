#include "Mod_Cir_Fusion_Main.h"
#include "Mod_Function.h"
#include "Mod_Read_Path.h"
#include <cstdlib>
#include <iostream>

int main()
{
	double TIME_S = Get_Time();
	Read_File();
	Circuit_Funsing_Main();
	double TIME_E = Get_Time();
	std::cout << "Total DPEC runtime:" << std::endl;
	Time_Diff(TIME_S, TIME_E);
	system("pause");
	return 0;
}
