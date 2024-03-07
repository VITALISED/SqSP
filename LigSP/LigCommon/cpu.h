/*-----------------------------------------------------------------------------
*
* File        : CPU.H
* Author      : Pavlos Touboulidis
* Description : CPU class to get MHz
*             : 
* Notes       : 
*             : 
*
*----------------------------------------------------------------------------*/
#ifndef	__CPU_H__
#define	__CPU_H__

#pragma warning ( push )
#pragma warning ( disable : 4035 )


class CCPU
{
private:
#ifdef WIN32
	static unsigned int	readTimeStampCounterLow()
	{
		_asm
		{
			_emit	0Fh		;RDTSC
			_emit	31h
		}
		//						returns eax
	}

	static unsigned	int	readTimeStampCounterHigh()
	{
		_asm
		{
			_emit	0Fh		;RDTSC
			_emit	31h
			mov	eax, edx ;eax=edx
		}
		//						returns eax
	}

	static unsigned	__int64	readTimeStampCounter()
	{
		_asm
		{
			_emit	0Fh		;RDTSC
			_emit	31h
		}
		//						returns edx:eax
	}

	static void readTimeStampCounter(unsigned int *uHigh, unsigned int *uLow)
	{
		_asm
		{
			_emit	0Fh		;RDTSC
			_emit	31h

			mov	ebx, uHigh
			mov	dword ptr [ebx], edx
			mov	ebx, uLow
			mov	dword ptr [ebx], eax
		}
	}
#endif
public:
	unsigned int tH;
	unsigned int tL;

	void start_clock()
	{
	#ifdef WIN32
		readTimeStampCounter(&tH,&tL);
	#endif
	}

	int get_timespan()
	{
	#ifdef WIN32
		unsigned int H;
		unsigned int L;
		readTimeStampCounter(&H,&L);
		if(H==tH) return (L-tL);
		else return(tL-L);
	#else
		return 0;
	#endif
	}
	
};

static CCPU CpuClock;

#pragma warning ( pop )



#endif	//__CPU_H__
