#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */
unsigned int cnt_mb_voice = 0; /*contador tramas en mb_voice*/
unsigned int cnt_mb_silence = 0;  /*contador tramas en mb_silence*/
unsigned int cnt_th_init = 0; /*contador tramas para threshold en init*/
float accum_power = 0; /*variable que acumula potencia del inicio*/
const float fm = 16000;
/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  feat.zcr = compute_zcr(x, N, fm);
  feat.p=compute_power(x, N);
  feat.am=compute_am(x, N);

  return feat;
}

/* 
 * DONE: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate, float alpha1) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->alpha1=alpha1;
  vad_data->k0 = 5; 
  vad_data->k1 = 5;
  vad_data->pPot = 0.989; /*trigger in percent in INIT state to detect voice*/ //0.989
  /*minimum number of stable frames*/
  vad_data->nStableInit = 7; //7
  vad_data->nStableVoice = 0;
  vad_data->nStableSilence = 9; //9

  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = ST_SILENCE; 
  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * DONE: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * DONE: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {
  case ST_INIT:
    accum_power = f.p + accum_power;
    cnt_th_init++;
    if(f.p >= (vad_data->pPot)*(accum_power/cnt_th_init) && (vad_data->nStableInit < cnt_th_init)){ //pPot - nStableInit
      vad_data-> p1 = (accum_power/cnt_th_init) + vad_data->k0;   //k0
      vad_data->state = ST_SILENCE; 
      cnt_th_init = 0;
    }
    break;

  case ST_MB_VOICE:
    if (f.p >= vad_data->p1 + vad_data->k1){ //k1 - nStableVoice
      if(cnt_mb_voice >= vad_data->nStableVoice){
        vad_data->state = ST_VOICE;
        cnt_mb_voice = 0;
      }else{
        cnt_mb_voice++;
      }
    }else
     vad_data->state = ST_SILENCE;

    break;

  case ST_MB_SILENCE:
    if (f.p <= vad_data->p1){ //nStableSilence
      if(cnt_mb_silence >= vad_data->nStableSilence){
      vad_data->state = ST_SILENCE;
      cnt_mb_silence = 0;
      }else{
        cnt_mb_silence++;
      }
    }else if(f.p > vad_data->p1)
      vad_data->state = ST_VOICE;
    break;

  case ST_SILENCE:
    if (f.p > vad_data->p1)
      vad_data->state = ST_MB_VOICE;

    break;

  case ST_VOICE:
    if (f.p < vad_data->p1)
      vad_data->state = ST_MB_SILENCE;
    break;

  case ST_UNDEF:
    break;

  }

  if (vad_data->state == ST_SILENCE ||vad_data->state == ST_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
