#include "sync.h"

extern struct TempObject sync_obj[MAX_SYNC_OBJ];

extern struct AdjZone zstable[8];

BinaryCharWriter::BinaryCharWriter(char *str) : BasicBinaryWriter(false)
{
		ptr=str;
}

Result BinaryCharWriter::flush()
{
		Int32 count;
		static int p=0;
		int z = 0;
		for(Size i=0;i<bufferCount;i++)
		{
			count=buffers[i].length;
			memmove(ptr + z, buffers[i].data, count);
			z += count;
		}
		bytecount = z;
		return AS_OK;
}

BinaryCharReader::BinaryCharReader(char *str) : BasicBinaryReader(NULL)
{
		p = 0;
		ptr=str;
}
Result BinaryCharReader::readBytes(void *buf, Size len)
{
		//static int p = 0;
		memcpy(buf, ptr+p, len);
		p+= len;

		return AS_OK;
}

static int xdlt_mmfile_outf(void *priv, mmbuffer_t *mb, int nbuf)
{
	mmfile_t *mmf = (mmfile_t *)priv;

	if (xdl_writem_mmfile(mmf, mb, nbuf) < 0)
	{
		return -1;
	}

	return 0;
}

void *wrap_malloc(void *priv, unsigned int size)
{
	return malloc(size);
}
void wrap_free(void *priv, void *ptr)
{
	free(ptr);
}
void *wrap_realloc(void *priv, void *ptr,unsigned int size)
{
	return realloc(ptr,size);
}

void my_init_xdiff(void)
{
	memallocator_t malt;
	malt.priv=NULL;
	malt.malloc = wrap_malloc;
	malt.free = wrap_free;
	malt.realloc = wrap_realloc;
	xdl_set_allocator(&malt);
}

mmfile_t ObjectDiff( char *obj1, int size1, char *obj2 , int size2 )
{
	mmfile_t mtpatch;
	xdl_init_mmfile(&mtpatch, std::max(size1,size2), XDL_MMF_ATOMIC);

	mmfile_t mt1,mt2;
	xdl_init_mmfile( &mt1, size1, XDL_MMF_ATOMIC );
	xdl_init_mmfile( &mt2, size2, XDL_MMF_ATOMIC );
	xdl_write_mmfile( &mt1 , obj1 , size1 );
	xdl_write_mmfile( &mt2 , obj2 , size2 );

	bdiffparam_t bdp;
	bdp.bsize = 64;

	xdemitcb_t ecb;
	ecb.priv = &mtpatch;
	ecb.outf = xdlt_mmfile_outf;

	xdl_seek_mmfile( &mt2 , 0 );
	xdl_seek_mmfile( &mt1 , 0 );
	if(xdl_bdiff(&mt1, &mt2, &bdp, &ecb) < 0 )
	{
		xdl_free_mmfile(&mt2);
		xdl_free_mmfile(&mt1);
		printf("diff false\n");
	}

	xdl_seek_mmfile( &mtpatch , 0 );
	char buf[200];
	xdl_read_mmfile( &mtpatch , buf , xdl_mmfile_size(&mtpatch) );
	mmfile_t mp;
	xdl_init_mmfile( &mp , xdl_mmfile_size(&mtpatch) , XDL_MMF_ATOMIC );
	xdl_write_mmfile( &mp , buf , xdl_mmfile_size(&mtpatch) );
	xdl_seek_mmfile(&mp, 0);

	xdl_read_mmfile( &mp , buf , xdl_mmfile_size( &mp ) );
	xdl_seek_mmfile(&mp, 0);
	xdl_free_mmfile(&mt2);
	xdl_free_mmfile(&mt1);
	xdl_free_mmfile(&mtpatch);
	return mp;
}

bool ObjectPatch( mmfile_t patch, char *oldobj, int *oldlen )
{
	mmfile_t origin,mped;
	
	xdl_init_mmfile( &origin, *oldlen, XDL_MMF_ATOMIC );
	xdl_init_mmfile( &mped, *oldlen, XDL_MMF_ATOMIC );
	xdl_write_mmfile( &origin , oldobj , *oldlen );
	xdl_seek_mmfile( &origin , 0 );

	xdemitcb_t ecb;
	ecb.priv = &mped;
	ecb.outf = xdlt_mmfile_outf;

	if (xdl_bpatch(&origin, &patch, &ecb) < 0)
	{
		xdl_free_mmfile(&patch);
		xdl_free_mmfile(&origin);
		xdl_free_mmfile(&mped);
		return false;
	}
	xdl_seek_mmfile( &mped , 0 );

	*oldlen = xdl_mmfile_size( &mped );

	xdl_read_mmfile( &mped , oldobj , *oldlen );
	xdl_free_mmfile(&patch);
	xdl_free_mmfile(&origin);
	xdl_free_mmfile(&mped);

	return true;
}

void IniObj( void )
{
	for(int i = 0 ; i < MAX_SYNC_OBJ ;++i)
	{
		sync_obj[i].uid = -1;
	}
}

int FindObj( int uid )
{
	for(int i = 0 ; i < MAX_SYNC_OBJ ;++i)
	{
		if( sync_obj[i].uid == uid )
			return i;
	}
	return -1;
}

int CreateObj( int uid, char *obj, int size )
{
	for(int i = 0 ; i < MAX_SYNC_OBJ ;++i)
	{
		if( sync_obj[i].uid == -1 )
		{
			sync_obj[i].obj = (char *)malloc(size);
			memcpy( sync_obj[i].obj , obj , size );
			sync_obj[i].size = size;
			sync_obj[i].uid = uid;
			return i;
		}
	}

	return -1;
}

void UpdateObj( int i,char *obj, int size, int time)
{
	sync_obj[i].obj = (char *)malloc(size);
	memcpy( sync_obj[i].obj , obj , size );
	sync_obj[i].size = size;
	sync_obj[i].time = time;
}

int DelObj( int uid )
{
	for(int i = 0 ; i < MAX_SYNC_OBJ ;++i)
	{
		if( sync_obj[i].uid == uid )
		{
			sync_obj[i].uid = -1;
			free(sync_obj[i].obj);
			return i;
		}
	}

	return -1;
}

void PrintObj( void )
{
	for(int i = 0 ; i < MAX_SYNC_OBJ ;++i)
	{
		if( sync_obj[i].uid == -1 )
		{
			printf("sync_obj[%d] is not used\n",i);
		}else
			printf("sync_obj[%d]'s uid = %d, obj's size = %d\n",i,sync_obj[i].uid,sync_obj[i].size);
	}
}