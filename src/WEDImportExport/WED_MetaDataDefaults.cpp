#include "WED_MetaDataDefaults.h"
#include "WED_Airport.h"
#include "WED_MetaDataKeys.h"
#include "WED_Menus.h" // for wed_AddMetaDataBegin and wed_AddMetaDataEnd
#include "CSVParser.h"
#include <fstream>

#if DEV
#define FROM_DISK 0
	#if FROM_DISK
	#define CSV_ON_DISK "C://airport_metadata.csv"	
	#endif
#endif

bool	fill_in_airport_metadata_defaults(WED_Airport & airport, const string& file_path)
{
#if DEV && FROM_DISK
	std::ifstream t(CSV_ON_DISK);
#else
	std::ifstream t(file_path.c_str());
#endif

	if(t.bad() == true)
	{
		t.close();
		return false;
	}

	std::string str((std::istreambuf_iterator<char>(t)),
					std::istreambuf_iterator<char>());
	
	CSVParser::CSVTable table = CSVParser(',', str).ParseCSV();
	
	t.close();
	return fill_in_airport_metadata_defaults(airport, table);
}

bool fill_in_airport_metadata_defaults(WED_Airport & airport, const CSVParser::CSVTable &table)
{
	CSVParser::CSVTable::CSVRow default_values;

	//Find the airport in the table match
	string icao;
	airport.GetICAO(icao);
	
	int i = 0;
	for ( ; i < table.GetRows().size(); ++i)
	{
		if(table.GetRows()[i][0] == icao)
		{
			default_values = table.GetRows()[i];
			break;
		}
	}

	//We hit the end
	if(i < table.GetRows().size() == false)
	{
		return false;
	}

	//For every column (excluding airport_id), copy if missing key or key's value is ""
	CSVParser::CSVTable::CSVHeader column_headers = table.GetHeader();
	for (i = 1; i < default_values.size(); i++)
	{
		string key = column_headers[i];
		string default_value = default_values[i];
		
		const KeyEnum key_enum = META_KeyEnumFromName(key);
		if(key_enum > wed_AddMetaDataBegin && key_enum < wed_AddMetaDataEnd) // We *only* want to insert keys we recognize
		{
			//For every of our column do They (airport) have this?
			bool has_key = airport.ContainsMetaDataKey(key);
			if(has_key)
			{
				if(airport.GetMetaDataValue(key) == "")
				{
					airport.EditMetaDataKey(key,default_value);
				}
			}
			else
			{
				airport.AddMetaDataKey(key, default_value);
			}
		}
	}
	return true;
}
