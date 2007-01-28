#include "WED_Package.h"
#include "PlatformUtils.h"
#include "MemFileUtils.h"
#include "FileUtils.h"
//#include "XFileTwiddle.h"
#include "WED_Document.h"
#include "GISUtils.h"
#include "WED_Errors.h"
#include "WED_Messages.h"

#define		EDIT_DIR_NAME		DIR_STR "WED"
#define		EARTH_DIR_NAME		DIR_STR "Earth nav data" 

inline int lon_lat_to_idx  (int lon, int lat) { return lon + 180 + 360 * (lat + 90); }
inline int lon_lat_to_idx10(int lon, int lat) { return (lon/10) + 18  +  36 * ((lat/10) +  9); }

static const char * ext = NULL;

static bool	scan_folder_exists(const char * name, bool dir, unsigned long long mod, void * ref)
{
	char * dirs = (char *) ref;
	if (dir)
	if (strlen(name) == 7 && 
		(name[0] == '+' || name[0] == '-') &&
		(name[3] == '+' || name[3] == '-') &&
		isdigit(name[1]) && 
		isdigit(name[2]) &&
		isdigit(name[4]) && 
		isdigit(name[5]) &&
		isdigit(name[6]))
	{
		int lat = (name[1]-'0')*10+(name[2]-'0');
		if (name[0]=='-') lat = -lat;
		int lon = (name[4]-'0')*100+(name[5]-'0')*10+(name[6]-'0');
		if (name[3]=='-') lon = -lon;
		if (lat >= -90 && lat < 90 && lon >= -180 && lon < 180)
			dirs[lon_lat_to_idx10(lon, lat)] = true;
		
	}	
	return true;
}

static bool	scan_file_exists(const char * name, bool dir, unsigned long long mod, void * ref)
{
	unsigned long long * dirs = (unsigned long long *) ref;
	if (!dir)
	if (strlen(name) == 11 && strcmp(name+7, ext) == 0 &&
		(name[0] == '+' || name[0] == '-') &&
		(name[3] == '+' || name[3] == '-') &&
		isdigit(name[1]) && 
		isdigit(name[2]) &&
		isdigit(name[4]) && 
		isdigit(name[5]) &&
		isdigit(name[6]))
	{
		int lat = (name[1]-'0')*10+(name[2]-'0');
		if (name[0]=='-') lat = -lat;
		int lon = (name[4]-'0')*100+(name[5]-'0')*10+(name[6]-'0');
		if (name[3]=='-') lon = -lon;
		if (lat >= -90 && lat < 90 && lon >= -180 && lon < 180)
		{
			dirs[lon_lat_to_idx(lon, lat)] = mod;
		}
		
	}	
	return true;
}

WED_Package::WED_Package(const char * inPath, bool inCreate)
{
	Assert(inPath[strlen(inPath)-1] != DIR_CHAR);
	mPackageBase = inPath;
	memset(mTiles, 0, sizeof(mTiles));

	if (inCreate)
	{
		string wed_folder = mPackageBase + EDIT_DIR_NAME + EARTH_DIR_NAME;
		string earth_folder = mPackageBase + EARTH_DIR_NAME;
		int err;
		if (err = FILE_make_dir_exist(mPackageBase.c_str()))
			WED_ThrowPrintf("Unable to create directory %s: %d", mPackageBase.c_str(), err);
		if (err = FILE_make_dir_exist(wed_folder.c_str()))
			WED_ThrowPrintf("Unable to create directory %s: %d", wed_folder.c_str(), err);
		if (err = FILE_make_dir_exist(earth_folder.c_str()))
			WED_ThrowPrintf("Unable to create directory %s: %d", earth_folder.c_str(), err);
	}
	
	Rescan();		
}

WED_Package::~WED_Package()
{
	for (int n = 0; n < 360 * 180; ++n)
	if (mTiles[n])
		delete mTiles[n];

	BroadcastMessage(msg_PackageDestroyed, 0);
}

int				WED_Package::GetTileStatus(int lon, int lat)
{
	return mStatus[lon_lat_to_idx(lon, lat)];
}

WED_Document *	WED_Package::GetTileDocument(int lon, int lat)
{
	return mTiles[lon_lat_to_idx(lon, lat)];
}

WED_Document *	WED_Package::OpenTile(int lon, int lat)
{
	double bounds[4] = { lon, lat, lon + 1, lat + 1 };
	char partial[30];
	sprintf(partial, DIR_STR "%+03d%+04d" DIR_STR "%+03d%+04d.xes", latlon_bucket(lat),latlon_bucket(lon),lat,lon);
	string path = mPackageBase + EDIT_DIR_NAME + EARTH_DIR_NAME + partial;

	sprintf(partial, DIR_STR "%+03d%+04d", latlon_bucket(lat),latlon_bucket(lon));
	string parent = mPackageBase + EDIT_DIR_NAME + EARTH_DIR_NAME + partial;
	int err;
	if (err = FILE_make_dir_exist(parent.c_str()))
		WED_ThrowPrintf("Unable to open create %s: %d", parent.c_str(), err);

	WED_Document * tile = new WED_Document(path, this, bounds);
	mTiles[lon_lat_to_idx(lon, lat)] = tile;
	#if !DEV
	revisit - is opening and creating a tile really different?>?
	#endif
//	tile->Load();
	return tile;
}

WED_Document *	WED_Package::NewTile(int lon, int lat)
{
	double bounds[4] = { lon, lat, lon + 1, lat + 1 };
	char partial[30];
	sprintf(partial, DIR_STR "%+03d%+04d" DIR_STR "%+03d%+04d.xes", latlon_bucket(lat),latlon_bucket(lon),lat,lon);
	string path = mPackageBase + EDIT_DIR_NAME + EARTH_DIR_NAME + partial;

	WED_Document * tile = new WED_Document(path, this, bounds);
	mTiles[lon_lat_to_idx(lon, lat)] = tile;
	return tile;
}


	
void			WED_Package::Rescan(void)
{
	unsigned long long 		dsf[360*180] = { 0 };
	unsigned long long 		xes[360*180] = { 0 };
	
	char					dsf_dir[36*18] = { 0 };
	char					xes_dir[36*18] = { 0 };
	
	string	dsf_dir_str	= mPackageBase + EARTH_DIR_NAME;
	string	xes_dir_str	= mPackageBase + EDIT_DIR_NAME + EARTH_DIR_NAME;
	
	MF_GetDirectoryBulk(dsf_dir_str.c_str(), scan_folder_exists, dsf_dir);
	MF_GetDirectoryBulk(xes_dir_str.c_str(), scan_folder_exists, xes_dir);
	
	int lon, lat;
	
	for (lat = -90; lat < 90; lat+=10)
	for (lon = -180; lon < 180; lon+=10)
	{
		char dname[25];
		sprintf(dname,"/%+03d%+04d" DIR_STR, lat, lon);
		string dsf_subdir_str = dsf_dir_str + dname;
		string xes_subdir_str = xes_dir_str + dname;
		
		ext=".dsf";		
		if (dsf_dir[lon_lat_to_idx10(lon,lat)])	MF_GetDirectoryBulk(dsf_subdir_str.c_str(), scan_file_exists, dsf);
		ext=".xes";
		if (xes_dir[lon_lat_to_idx10(lon,lat)])	MF_GetDirectoryBulk(xes_subdir_str.c_str(), scan_file_exists, xes);
		ext=NULL;
	}
	
	for (lat = -90 ; lat <  90; ++lat)
	for (lon = -180; lon < 180; ++lon)
	{
		int idx = lon_lat_to_idx(lon, lat);
		
		if (dsf[idx] != 0)
		{
			if (xes[idx] != 0)
			{
				mStatus[idx] = (dsf[idx] > xes[idx]) ? status_UpToDate : status_Stale;
			} else {
				mStatus[idx] = status_DSF;
			}
		} else {
			if (xes[idx] != 0)
			{
				mStatus[idx] = status_XES;
			} else {
				mStatus[idx] = status_None;
			}
		}
	}
}

