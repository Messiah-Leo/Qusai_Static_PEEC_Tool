#include "Mod_Connection.h"
#include "Mod_Element_Build.h"
#include "Mod_Gaussian.h"
#include "Mod_Read_File.h"

int main() {

	Ini_Gaussion();
	Read_File();
	Creat_Connection();
	Element_Build();
	system("pause");
}