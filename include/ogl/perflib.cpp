/******************* Performance Data Collection ***********************
  Network, CPU, and GPU performance data collection function.  
  
  For a solid benchmark:
  	- call perf_init between glutInit and glutCreateWindow. 
  	- call perf_startframe at the start of each frame.  This starts a timer.
	- call perf_swapframe to swap buffers (instead of glutSwapBuffers())
	- call glutPostRedisplay from your idle function.
*/
#ifndef USER_HZ
#define USER_HZ 100 /* <- Linux default.  The real header isn't easy to get from outside the kernel. */
#endif
#include <stdlib.h> /* for "getenv" */
#include <stdio.h> /* for printf & friends */
#include <unistd.h> /* for "unlink" */

#define INTERNAL /* empty, could be static though */


INTERNAL int perf_framecount=0; /* frame counter, used for performance analysis */
INTERNAL int perf_rank=0, perf_size=1; /* MPI rank and size */
INTERNAL double perf_dim_x=0,perf_dim_y=0;

/**
  Accumulators for time spent doing various stuff:  
	CPU time between startframe and swapframe
	GPU time in swapframe waiting on glFinish
	swap time in swapframe waiting on glutSwapBuffers
	measure time inside perf_data_get munging kernel data structures
*/
INTERNAL double perf_accum_CPU=0.0; /* time spent executing code in CPU (s) */
INTERNAL double perf_accum_GPU=0.0; /* time spent by CPU waiting for GPU (s) */
INTERNAL double perf_accum_swap=0.0; /* time spent waiting for buffer swap (s) [MPIglut load imbalance] */
INTERNAL double perf_accum_tween=0.0; /* time spend between frames, in GLUT event handling */
INTERNAL double perf_accum_measure=0.0; /* total time spent measuring performance (s) */
#define perf_n_accum 5 /* number of accumulator values above */

typedef struct {
	double rank; /* MPI rank of reporting CPU */
	double time; /* wall-clock time data was aquired, seconds */
	double frames; /* total number of frames drawn so far (from a global variable count) */
	double cpu_usage, idle_time; /* aggregate CPU usage and idle time, seconds */
	double net_tx, net_rx; /* sent and received bytes on the network */
	double mem; /* memory usage, kilobytes */
	double accum[perf_n_accum];
} perf_data;


/** Return wall-clock time, in seconds */
double perf_time(void) {return 0.001*glutGet(GLUT_ELAPSED_TIME); }

/** Open this file or else abort */
FILE *fopen_dangit(const char *fname,const char *mode) 
{
	FILE *f=fopen(fname,mode);
	if (f==NULL) {
		printf("perflib: Error opening file '%s'\n",fname); 
		abort();
	}
	return f;
}

/**
  Given:
  	cur, the current 32-bit monotonic-counter value
	last, the previous 32-bit monotonic-counter value
	wraps, a double-precision counter wraparound count
  return the actual value of the counter, including wraparound.
*/
INTERNAL double counter_plus_wraparound(unsigned int cur,unsigned int *last,double *wraps)
{
	if (cur<*last) { /* counter *decreased*: must have wrapped around */
		*wraps+=(unsigned int)(-1); *wraps+=1; /* add 2^32 to "wraps" */
	}
	*last=cur; /* update last value */
	return cur+*wraps;
}

void perf_data_print( perf_data *data,FILE *out, int with_header) 
{
	int i;

	if (with_header) 
		fprintf(out,"Rank\ttime\tfps\tframes\t"
			"CPU\tGPU\tswap\ttween\tMeasure\t"
			"X\tY\t"
			"oscpu\tosidle\tnet_tx\tnet_rx\tmem\n");
	fprintf(out,"%.0f\t",data->rank);
	fprintf(out,"%0.2f\t",data->time);  /* seconds total runtime */
	fprintf(out,"%0.2f\t",data->frames/data->time); /* frames per second */
	fprintf(out,"%.0f\t",data->frames);      /* frame count */
	for (i=0;i<perf_n_accum;i++)
		fprintf(out,"%.4f\t",data->accum[i]/data->time); /* fraction */
	fprintf(out,"%.0f\t",perf_dim_x); /* pixels */
	fprintf(out,"%.0f\t",perf_dim_y); /* pixels */
	fprintf(out,"%.4f\t",data->cpu_usage/data->time); /* fraction */
	fprintf(out,"%.4f\t",data->idle_time/data->time); /* fraction */
	fprintf(out,"%.3f\t",1.0e-6*data->net_tx/data->time); /* MB/s */
	fprintf(out,"%.3f\t",1.0e-6*data->net_rx/data->time); /* MB/s */
	fprintf(out,"%.3f\t",1.0e-3*data->mem);	 /* MB */
	fprintf(out,"\n");
	fflush(out);
}

void perf_data_get( perf_data *data )
{
	unsigned int cpu_usage, idle_time; /* aggregate CPU usage and idle time, 1/100 seconds */
	unsigned int net_tx, net_rx; /* sent and received bytes on the network */
	static unsigned int tx_last=0, rx_last=0; /* to detect wraparound */
	static double tx_wrap=0, rx_wrap=0; /* to detect wraparound */
	unsigned int mem; /* memory usage, kilobytes */
	char statFile[1024], cmd[1024], sillystr[1024];
	double perf_start_time=perf_time();
	
	FILE *f;
	sprintf(statFile,"/tmp/mpiglut_stat_%d",perf_rank);
/*
 For /proc/stat info see: http://www.linuxhowtos.org/System/procstat.htm.
 Annoyingly, the below takes 15ms to complete on powerwall0.
*/
	sprintf(cmd,"echo %d `cat /proc/stat | grep 'cpu ' | awk '{print $2, $5}'`  `cat /proc/%d/status | grep 'VmSize' | awk -F: '{print $2}' | awk '{print $1}'`  `/sbin/ifconfig | grep 'RX bytes' | head -1 | awk -F: '{print $2, $3}' | awk '{print $1, $6}'` SILLYCRAP > %s",perf_rank,getpid(),statFile);
	system(cmd);
	if (0) { /* for debugging, print the status file */
		sprintf(statFile,"cat %s",statFile);
		system(statFile);
	}
	f=fopen_dangit(statFile,"r"); 
	int readRank=0;
	fscanf(f,"%u%u%u%u%u%u%s",
		&readRank,&cpu_usage,&idle_time,&mem,&net_rx,&net_tx,sillystr);
	if (readRank!=perf_rank || 0!=strcmp(sillystr,"SILLYCRAP")) {
		printf("perflib: Error reading performance stats file on rank %d (first int %d, last string '%s')\n",
			perf_rank,readRank,sillystr); 
		abort();
	}
	fclose(f);
	unlink(statFile);
	
	perf_accum_measure+=perf_time()-perf_start_time;
	
	data->rank = perf_rank;
	data->time = perf_time();
	data->frames = perf_framecount;
	data->cpu_usage = cpu_usage*(1.0/USER_HZ);
	data->idle_time = idle_time*(1.0/USER_HZ);
	data->net_tx = counter_plus_wraparound(net_tx,&tx_last,&tx_wrap);
	data->net_rx = counter_plus_wraparound(net_rx,&rx_last,&rx_wrap);
	data->mem = mem;
	data->accum[0]=perf_accum_CPU;
	data->accum[1]=perf_accum_GPU;
	data->accum[2]=perf_accum_swap;
	data->accum[3]=perf_accum_tween;
	data->accum[4]=perf_accum_measure;
	
	if (0) { /* for debugging, print the collected data */
		perf_data_print(data,stdout,1);
	}
}

perf_data perf_data_diff(perf_data *start,perf_data *end)
{
	perf_data r; int i;
	
	r.rank = end->rank;
	r.time = end->time - start->time;
	r.frames = end->frames - start->frames;
	r.cpu_usage = end->cpu_usage - start->cpu_usage;
	r.idle_time = end->idle_time - start->idle_time;
	r.net_tx = end->net_tx - start->net_tx;
	r.net_rx = end->net_rx - start->net_rx;
	r.mem = end->mem; //  - start->mem;
	for (i=0;i<perf_n_accum;i++)
		r.accum[i]=end->accum[i]-start->accum[i];
	
	return r;
}

/** Interface to performance tracing code */
int perf_firstframe=-1, perf_nextframe=-1, perf_perframe=100, perf_nout=5;
double perf_startframe_time=0, perf_tweenframe_time=0;
INTERNAL perf_data perf_last;

/** Call this function to set up the perforance run.
*/
void perf_init(void) {
	FILE *sz, *measure;
#ifdef MPIGLUT_H
	MPI_Comm_rank(MPI_COMM_WORLD, &perf_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &perf_size);
#endif

	sz=fopen("perf_size","r");
	if (sz) {
		int w=-1,h=-1;
		fscanf(sz,"%d%d",&w,&h);
		if (perf_rank==0) printf("perflib: setting window size to %d x %d\n",w,h);
		glutInitWindowSize(w,h);
		glutInitWindowPosition(0,0);
		perf_dim_x=w; perf_dim_y=h;
		fclose(sz);
	}
	
	measure=fopen("perf_measure","r");
	if (measure) {
		glutIdleFunc(glutPostRedisplay); /* need lots of frames for performance analysis */
		perf_firstframe=10;
		fscanf(measure,"%d%d%d",
			&perf_firstframe,&perf_perframe,&perf_nout);
		if (perf_rank==0) printf("perflib: will start output at frame %d, then every %d frames, for %d times\n",
			perf_firstframe,perf_perframe,perf_nout);
		fclose(measure);
	}
	if (perf_rank==0) fflush(stdout);
}

/** Call this function at the start of every frame */
void perf_startframe(void) {
	perf_startframe_time=perf_time();
	if (perf_tweenframe_time!=0) {
		perf_accum_tween+= perf_startframe_time - perf_tweenframe_time;
		perf_tweenframe_time=0;
	}
}

/** Call this function at the end of every frame, instead of glutSwapBuffers. 
*/
void perf_swapframe(void) {
	double perf_start=perf_time(), start_swap, end_swap;
/* Every-frame stuff: */
	if (perf_startframe_time==0) {printf("perflib: Never called perf_startframe!\n"); abort();}
	perf_accum_CPU += perf_start-perf_startframe_time;
	glFinish(); /* time spent waiting for GPU */
	start_swap=perf_time();
	perf_accum_GPU += start_swap-perf_start;
	glutSwapBuffers();
	end_swap=perf_time();
	perf_accum_swap += end_swap-start_swap;
	glFinish(); /* time spent waiting for GPU to finish flipping its buffers */
	perf_accum_GPU += perf_time()-end_swap;
	
/* Not-every-frame stuff: */
	perf_framecount++;
	if (perf_firstframe==perf_framecount) {
		if (perf_rank==0) printf("perflib: Starting performance data recording at frame %d...\n",perf_framecount);
		perf_data_get(&perf_last);
		perf_nextframe=perf_framecount+perf_perframe;
	} else if (perf_nextframe==perf_framecount) {
		static int perf_out=0;
		perf_data cur,diff;
		perf_data_get(&cur);
		diff=perf_data_diff(&perf_last,&cur);
		
		/* write out recorded data */
		#define perf_collect_tag 0x6e7f
		#define perf_collect_master 0
		if (perf_rank==perf_collect_master) {
			int src;
			char fname[1000];
			FILE *f;
			system("mkdir -p perf");
			sprintf(fname,"perf/%d_%d",perf_size,perf_out);
			printf("perflib: writing frame %d performance data to file '%s'\n",
				perf_framecount,fname);
			fflush(stdout);
			f=fopen_dangit(fname,"w");
			for (src=0;src<perf_size;src++) {
				perf_data to_print;
				if (src!=perf_collect_master) {
			#if MPIGLUT_H
					MPI_Recv(&to_print,sizeof(to_print)/sizeof(double),MPI_DOUBLE,
						src,perf_collect_tag,MPI_COMM_WORLD,NULL);
			#endif
				} else to_print=diff;  /* write my own data */
				perf_data_print(&to_print,f,src==0);
			}
			fclose(f);
		} else {
			#if MPIGLUT_H
			MPI_Send(&diff,sizeof(diff)/sizeof(double),MPI_DOUBLE,
				perf_collect_master,perf_collect_tag,MPI_COMM_WORLD);
			#endif
		}
				
		if ((++perf_out)>=perf_nout) 
		{ /* we've run long enough-- time to exit! */
			if (perf_rank==0) printf("perflib: captured enough data.  Now exiting program.\n");
			#if MPIGLUT_H
			// MPI_Finalize();  //<- does GLUT do this automatically?
			#endif
			exit(0);
		}
		
		perf_last=cur;
		perf_nextframe=perf_framecount+perf_perframe;
	}
	perf_tweenframe_time=perf_time(); /* time spend in GLUT event handling */
}

