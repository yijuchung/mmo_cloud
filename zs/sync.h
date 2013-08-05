#include "zstable.h"
#include "autoserial\binaryhandlers.h"
#include "autoserial\autoserial.h"
//#include "autoserial/autoserial_socket.h"
#include "user.h"
#include "xdiff\xdiff.h"
#include "def.h"

using namespace autoserial;

static int xdlt_mmfile_outf(void *priv, mmbuffer_t *mb, int nbuf);
void *wrap_malloc(void *priv, unsigned int size);
void wrap_free(void *priv, void *ptr);
void *wrap_realloc(void *priv, void *ptr,unsigned int size);
void my_init_xdiff(void);

#ifndef __AUTO__
#define __AUTO__
class BinaryCharWriter : public BasicBinaryWriter
{
	private:
		char *ptr;

	protected:
		virtual Result flush();

	public:
		int bytecount;
		BinaryCharWriter(char *str);
};

class BinaryCharReader : public BasicBinaryReader
{
	private:
		char *ptr;

	protected:
		virtual Result readBytes(void *buf, Size len);

	public:
		BinaryCharReader(char *str);
		int p;
};
#endif

struct TempObject
{
	int uid;
	char *obj;
	int size;
	int time;
};

void IniObj( void );

int FindObj( int uid );

int CreateObj( int uid, char *obj, int size );

int DelObj( int uid );

void UpdateObj( int i,char *obj, int size, int time);

void PrintObj( void );

//-------------------------------------------------------------

//-------------------------------------------------------------
// diff and return patch
mmfile_t ObjectDiff( char *obj1, int size1, char *obj2 , int size2 );

// object patch
bool ObjectPatch( mmfile_t patch, char *oldobj, int *oldlen );