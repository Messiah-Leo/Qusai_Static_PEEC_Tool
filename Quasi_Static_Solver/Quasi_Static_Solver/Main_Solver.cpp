#include "Mod_Read_File.h"
#include "Mod_Solver_Freq.h"
#include "Mod_Solver_Time.h"


int main() {
	Read_File();
	if (Solver_SET == 0)
		Freq_Solver();
	else if (Solver_SET == 1)
		Time_Solver();
}