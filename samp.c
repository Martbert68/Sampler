/* samp.c */
/* Simple Sampler*/

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

/* here are our X variables */
Display *display;
XColor    color[100];
int screen;
Window win;
GC gc;
unsigned long black,white;
#define X_SIZE 250 
#define Y_SIZE 550 
//ssize=44100*2*60;
#define SSIZE 52920000

/* here are our X routines declared! */
void init_x();
void close_x();
void redraw();

void slider(int,int,char *,int *);
void knob  (int,int,int,int *);
void button(int,int,char *,int *);
long load_chan(short *,int);

/* sound */
void *spkr();
void *control();
void *aux();

struct output { short *waveform; short *sample; long where; int vol[8]; int end[8]; int speed[8]; int gap[8]; short *buffer; int blen; int rec; int echovol;int echotime; int aut;long speedf; };

void usage ()
{
	printf("usage: samp\n");
	exit (1);
}

int main(int argc,char *argv[])
{
 
	int *fhead,len,chan,sample_rate,bits_pers,byte_rate,ba,size,ssize,init;
	struct output *out;
	long buffp[8];

        out=(struct output *)malloc(sizeof(struct output ));
        out->sample=(short *)malloc(sizeof(short )*SSIZE*8);

	for (init=0;init<8;init++)
	{
		buffp[init]=0;
		out->speed[init]=800;
		out->vol[init]=400;
	}
	out->echovol=0;
	out->echotime=44100;
	out->aut=1;
	out->speedf=0;


        len=60*60; //1 hour
        chan=2;
        sample_rate=44100;
        bits_pers=16;
        byte_rate=(sample_rate*chan*bits_pers)/8;
        ba=((chan*bits_pers)/8)+bits_pers*65536;
        size=chan*len*sample_rate;
        out->waveform=(short *)malloc(sizeof(short)*size);
        out->buffer=(short *)malloc(sizeof(short)*44100*2*60);


       pthread_t spkr_id,aux_id,control_id;

       struct timespec tim, tim2;
       tim.tv_sec = 0;
        tim.tv_nsec = 100L;

        pthread_create(&spkr_id, NULL, spkr, out);
        pthread_create(&control_id, NULL, control, out);
        pthread_create(&aux_id, NULL, aux, out);

	long my_point,ahead,chunk;
	chunk=800;
	ahead=800;
	my_point=2*ahead;


	while (1>0)
	{
		int wcount;
		long trigger;
		trigger=my_point-ahead;
		wcount=0;
		while (out->where<trigger)
		{
                       nanosleep(&tim , &tim2);
		       wcount++;

		}
		if (wcount < 20){printf ("%d\n",wcount);}


		long from,to,sum;
		from=my_point;
		to=my_point+chunk;

		for (my_point=from;my_point<to;my_point+=2)
		{
			int chan;
			out->waveform[my_point]=0; out->waveform[my_point+1]=0;
			for (chan=0;chan<8;chan++)
			{
				long sp_point;
				sp_point=(buffp[chan]*out->speed[chan])/800;
				if (sp_point>=out->end[chan]){
					if (out->aut==1){
						buffp[chan]=-(out->gap[chan])-((rand()*out->gap[chan])/127680);sp_point=0;out->speed[chan]+=out->speedf;
						if (out->speed[chan]>1600){out->speed[chan]=400;}}
					else
					{
						buffp[chan]=-SSIZE;sp_point=0;}
				}
				if (buffp[chan]>=0)
				{
				out->waveform[my_point]+=(out->vol[chan]*out->sample[sp_point+(chan*SSIZE)]/800);
				out->waveform[my_point+1]+=(out->vol[chan]*out->sample[sp_point+1+(chan*SSIZE)]/800);
				}
				if (out->gap[chan]==SSIZE){out->gap[chan]=0;buffp[chan]=0;out->speedf=0;}
				buffp[chan]+=2;
			}

			short echol,echor;
			echol=(out->echovol*out->waveform[my_point-out->echotime])/800;
			out->waveform[my_point]+=echol;

			echor=(out->echovol*out->waveform[my_point-out->echotime])/800;
			out->waveform[my_point+1]+=echor;
		}
	} 
}

void *control(void *o) {
        // This handles the speakers.

	init_x();
	int *lattice; 
        lattice=(int *)malloc(sizeof(int)*X_SIZE*Y_SIZE);

  struct output *out;
  out=(struct output *)o;
  int rec;
  long start;
  rec=0;
  char c;
  char text[255];
  XEvent event;           /* the XEvent declaration !!! */
  KeySym key;             /* a dealie-bob to handle KeyPress Events */

	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	knob   (60,32,0,lattice);
	slider (60,32,"vol",lattice);
	button (140,30,"rec",lattice);
	button (140,60,"mode",lattice);
	button (140,90,"chan 0",lattice);
	button (140,120,"load 0",lattice);
	button (140,150,"save",lattice);
	button (140,180,"auto",lattice);
	button (140,210,"rel",lattice);
	XFlush(display);

	int slide,mode,rect,chan,save,recno;
	long save_start;
	char chant[100];
	slide=0; mode=0; rec=0; chan=0; save=0;recno=0;

	while (1>0)
	{
		int x_point,y_point;
                XNextEvent(display, &event);
		//knob   (60,32,0,lattice);
		//slider (60,32,"start",lattice);
		//button (140,30,"rec",lattice);
		//button (140,60,"mode",lattice);
		//button (140,90,"chan 0",lattice);
                if (event.type==KeyPress && XLookupString(&event.xkey,text,255,&key,0)==1)
                {
                        //printf ("got %c \n",text[0]);
			//out->vamp=32760;

			switch (text[0])
			{
		        case 'q': exit (0); break;
		        case '1': out->gap[0]=SSIZE; break;
		        case '2': out->gap[1]=SSIZE; break;
		        case '3': out->gap[2]=SSIZE; break;
		        case '4': out->gap[3]=SSIZE; break;
		        case '5': out->gap[4]=SSIZE; break;
		        case '6': out->gap[5]=SSIZE; break;
		        case '7': out->gap[6]=SSIZE; break;
		        case '8': out->gap[7]=SSIZE; break;
			}
		}
		if (event.type==ButtonPress ) {
                        x_point=event.xbutton.x; y_point=event.xbutton.y;
			long point,loaded;
			point=lattice[(y_point*X_SIZE)+x_point];
			if (point==210) { out->gap[0]=SSIZE ;}  
			if (point==180) { if (out->aut==1){ out->aut=0;  
					button (140,180,"manual",lattice);
				}else{
					button (140,180,"auto",lattice);
					out->aut=1;

				}
			}
			if (point==150) { if (save==0){ 
						save=1;
						button (140,150,"saving",lattice);
						save_start=out->where;
					}else{
						save=0;
						button (140,150,"save",lattice);

				 	printf("Recording finished \n");
                                        int *fhead,chan,sample_rate,bits_pers,byte_rate,ba,size;
                                        fhead=(int *)malloc(sizeof(int)*11);
                                        char fname[200];

                                        chan=2;
                                        sample_rate=44100;
                                        bits_pers=16;
                                        byte_rate=(sample_rate*chan*bits_pers)/8;
                                        ba=((chan*bits_pers)/8)+bits_pers*65536;
                                        size=out->where-save_start;

                                        fhead[0]=0x46464952;
                                        fhead[1]=36;
                                        fhead[2]=0x45564157;
                                        fhead[3]=0x20746d66;
                                        fhead[4]=16;
                                        fhead[5]=65536*chan+1;
                                        fhead[6]=sample_rate;
                                        fhead[7]=byte_rate;
                                        fhead[8]=ba;
                                        fhead[9]=0x61746164;
                                        fhead[10]=(size*chan*bits_pers)/8;


                                        FILE *record;
                                        sprintf(fname,"recording%d.wav",recno);
                                        record=fopen(fname,"wb");
                                        fwrite(fhead,sizeof(int),11,record);
                                        fwrite(out->waveform+save_start,sizeof(short),size,record);
                                        fclose (record);
                                        free(fhead);
                                        recno++; 
					} 
			}
			if (point==120) { loaded=load_chan (out->buffer,chan);
				printf("loaded %ld \n",loaded);
				memcpy(out->sample+(chan*SSIZE),out->buffer,loaded*2);
				out->end[chan]=loaded;
			}
			if (point==32) { slide=1;}
			if (point==30) { rect=1;button (140,30,"recording",lattice); out->rec=1;}
			if (point==60) { 
				mode++;if( mode>5){mode=0;}
				switch (mode)
				{
				case 0: slider(60,32,"vol",lattice);break;
				case 1: slider(60,32,"gap",lattice);break;
				case 2: slider(60,32,"speed",lattice);break;
				case 3: slider(60,32,"echo vol",lattice);break;
				case 4: slider(60,32,"echo time",lattice);break;
				case 5: slider(60,32,"speed f",lattice);break;
				}
			}
			if (point==90) { chan++; if(chan>7){chan=0;}
				sprintf(chant,"chan %d",chan);
				button (140,90,chant,lattice);
				sprintf(chant,"load %d",chan);
				button (140,120,chant,lattice);
			}

		}
			       
                if (event.type==MotionNotify && slide)	{ 
                        x_point=event.xbutton.x; y_point=event.xbutton.y;
			if (y_point<432 && y_point>31){
			knob   (60,32,y_point-32,lattice);
			if (mode==5){ out->speedf=(y_point-32)/2;}
			if (mode==4){ out->echotime=(y_point-32)*1600;}
			if (mode==3){ out->echovol=(y_point-32)*4;}
			if (mode==2){ out->speed[chan]=(y_point-32)*4;}
			if (mode==1){ out->gap[chan]=(y_point-32)*1600;}
			if (mode==0){ out->vol[chan]=(y_point-32)*8;}
			}
		}
		if (event.type==ButtonRelease ) { slide=0;
			button (140,30,"rec",lattice);
			if (out->rec==1)
			{
				out->rec=0;
				memcpy(out->sample+(chan*SSIZE),out->buffer,out->blen*2);
				out->end[chan]=out->blen;
			}
		}

	}
}

long load_chan  (short *samp,int chan)
{
	FILE *loadfile;


        int *fhead,chans,sample_rate,bits_pers,byte_rate,size;
        fhead=(int *)malloc(sizeof(int)*11);
        char fname[200];
	long ba;

        chans=2;
        sample_rate=44100;
        bits_pers=16;
        byte_rate=(sample_rate*chans*bits_pers)/8;

        sprintf(fname,"record%d.wav",chan);
        loadfile=fopen(fname,"rb");
	if (loadfile !=NULL){
	printf ("loading %s\n",fname);
        fread(fhead,sizeof(int),11,loadfile);
        size=8*fhead[10]/(chans*bits_pers);
        size=2*fhead[10];
        size=SSIZE;
        ba=fread(samp,sizeof(short),size,loadfile);
        fclose (loadfile); }
        free(fhead);
	return ba ;
}


void slider (int x,int y, char *txt, int *lattice)
{
	int along,len;
	len=strlen(txt);
        XSetForeground(display,gc,white);
	XClearArea(display, win,0,y-20,100,20,0 );
        XDrawString(display,win,gc,x-(2*len),y-10,txt,len);
	for (along=y;along<y+400;along++)
	{
		XDrawPoint(display, win, gc, x, along);
	}

}
void button (int x,int y, char *txt, int *lattice)
{
	int ee,dd,len;
	len=strlen(txt);
        XSetForeground(display,gc,white);
	XClearArea(display, win,x+40,y+6,100,11,0 );
        XDrawString(display,win,gc,x+40,y+15,txt,len);
	for (ee=0;ee<20;ee++)
	{
		for (dd=0;dd<20;dd++)
		{	
			XDrawPoint(display, win, gc, x+dd , ee+y);
			lattice[((ee+y)*X_SIZE)+(x+dd)]=y;
		}
	}
}

void knob (int x,int y, int val, int *lattice)
{
	int along,len,dd,ee;
        XSetForeground(display,gc,black);
	for (along=y;along<y+412;along++)
	{
		for (dd=4;dd<20;dd++)
		{	
			XDrawPoint(display, win, gc, x-dd , along);
			XDrawPoint(display, win, gc, x+dd , along);
			lattice[(along*X_SIZE)+(x+dd)]=0;
			lattice[(along*X_SIZE)+(x-dd)]=0;
		}
	}
        XSetForeground(display,gc,white);
	for (ee=0;ee<12;ee++)
	{
		for (dd=4;dd<20;dd++)
		{	
			XDrawPoint(display, win, gc, x-dd , ee+y+val);
			XDrawPoint(display, win, gc, x+dd , ee+y+val);
			lattice[((ee+y+val)*X_SIZE)+(x+dd)]=y;
			lattice[((ee+y+val)*X_SIZE)+(x-dd)]=y;
		}
	}

}


void *aux (void *o) 
{

  struct output *out;
  out=(struct output *)o;

  int rc,point;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  short *buffer;

  // Open PCM device for recording (capture).  
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(handle, params);
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  snd_pcm_hw_params_set_channels(handle, params, 2);
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);


  // Write the parameters to the aux 
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1); }
 snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(handle, params);
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  snd_pcm_hw_params_set_channels(handle, params, 2);
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);

  // Set period size to 32 frames. 
  frames = 32;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  // Write the parameters to the aux 
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  // Use a buffer large enough to hold one period 
  snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);

  // We want to loop for 5 seconds 
  snd_pcm_hw_params_get_period_time(params,
                                        &val, &dir);

  size = frames * 4; // 2 bytes/sample, 2 channels 
  buffer = (short *) malloc(size);


  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 100L;

  int last;
  last=0;

  while (1 > 0) {
	if (last==1 && out->rec==0)
	{
		printf ("rec fin %d\n",out->blen);
		point=0;
		last=0;
	}
	if (out->rec==0)
	{
    		rc = snd_pcm_readi(handle, buffer, frames);
		//nanosleep(&tim , &tim2);
	}else{
    		rc = snd_pcm_readi(handle, out->buffer+point, frames);
		point+=(frames*2);
		out->blen=point;
		last=1;
	}
    	if (rc == -EPIPE) {
      	/* EPIPE means overrun */
      		fprintf(stderr, "overrun occurred\n");
      		snd_pcm_prepare(handle);
    	} else if (rc < 0) {
      		fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    	} else if (rc != (int)frames) {
      	fprintf(stderr, "short read, read %d frames\n", rc); }
    }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}

void *spkr(void *o) {
        // This handles the speakers.

  struct output *out;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;

  out=(struct output *)o;

  //char *buffer;

  /* Open PCM device for playback. */
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }
  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);
  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);
  /* Set the desired hardware parameters. */
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);
  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
  /* Set period size to 32 frames. */
  frames = 4;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
  /* Write the parameters to the aux */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  //size = frames * 4; /* 2 bytes/sample, 2 channels 
  // size as in number of data points along
  size = frames * 2;

  snd_pcm_hw_params_get_period_time(params, &val, &dir);

  while (1 > 0) {
    rc = snd_pcm_writei(handle, (out->waveform+out->where), frames);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
    out->where+=size;
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  //free(buffer);

  return 0;
}

void init_x()
{
/* get the colors black and white (see section for details) */
        XInitThreads();
        //x_buffer=(unsigned char *)malloc(sizeof(unsigned char)*4*VX_SIZE*VY_SIZE);
        display=XOpenDisplay((char *)0);
        screen=DefaultScreen(display);
        black=BlackPixel(display,screen),
        white=WhitePixel(display,screen);
        win=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0, X_SIZE, Y_SIZE, 5, white,black);
        XSetStandardProperties(display,win,"PC scope","PC scope",None,NULL,0,NULL);
        XSelectInput(display, win, ExposureMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|ButtonMotionMask);
        //XSelectInput(display, vwin, ExposureMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|ButtonMotionMask|);
        gc=XCreateGC(display, win, 0,0);
        XSetBackground(display,gc,black); XSetForeground(display,gc,white);
        XClearWindow(display, win); 
        XMapRaised(display, win);
        XMoveWindow(display, win,0,0);
        Visual *visual=DefaultVisual(display, 0);
        Colormap cmap;
        cmap = DefaultColormap(display, screen);
        color[0].red = 0; color[0].green = 0; color[0].blue = 0;

        color[1].red = 65535; color[1].green = 0; color[1].blue = 0;
        color[2].red = 0; color[2].green = 65535; color[2].blue = 0;
        color[3].red = 0; color[3].green = 0; color[3].blue = 65535;

        color[4].red = 0; color[4].green = 65535; color[4].blue = 65535;
        color[5].red = 65535; color[5].green = 65535; color[5].blue = 0;
        color[6].red = 65535; color[6].green = 0; color[6].blue = 65535;

        color[7].red = 32768; color[7].green = 65535; color[7].blue = 0;
        color[8].red = 65535; color[8].green = 32768; color[8].blue = 0;
        color[9].red = 0; color[9].green = 65535; color[9].blue = 32768;

        color[10].red = 65535; color[10].green = 65535; color[10].blue = 65535;
        color[11].red = 65535; color[11].green = 32768; color[11].blue = 65535;
        color[12].red = 32768; color[12].green = 65535; color[12].blue = 65535;

        color[13].red = 32768; color[13].green = 0; color[13].blue = 0;
        color[14].red = 0; color[14].green = 32768 ; color[14].blue = 0;
        color[15].red = 45535; color[15].green = 45535 ; color[15].blue = 45535;

        XAllocColor(display, cmap, &color[0]);
        XAllocColor(display, cmap, &color[1]);
        XAllocColor(display, cmap, &color[2]);
        XAllocColor(display, cmap, &color[3]);
        XAllocColor(display, cmap, &color[4]);
        XAllocColor(display, cmap, &color[5]);
        XAllocColor(display, cmap, &color[6]);
        XAllocColor(display, cmap, &color[7]);
        XAllocColor(display, cmap, &color[8]);
        XAllocColor(display, cmap, &color[9]);
        XAllocColor(display, cmap, &color[10]);
        XAllocColor(display, cmap, &color[11]);
        XAllocColor(display, cmap, &color[12]);
        XAllocColor(display, cmap, &color[13]);
        XAllocColor(display, cmap, &color[14]);
        XAllocColor(display, cmap, &color[15]);
}

