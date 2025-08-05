void LOGGER_init__(LOGGER *data__, BOOL retain) {
  __INIT_VAR(data__->EN,__BOOL_LITERAL(TRUE),retain)
  __INIT_VAR(data__->ENO,__BOOL_LITERAL(TRUE),retain)
  __INIT_VAR(data__->TRIG,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->MSG,__STRING_LITERAL(0,""),retain)
  __INIT_VAR(data__->LEVEL,LOGLEVEL__INFO,retain)
  __INIT_VAR(data__->TRIG0,__BOOL_LITERAL(FALSE),retain)
}

// Code part
void LOGGER_body__(LOGGER *data__) {
  // Control execution
  if (!__GET_VAR(data__->EN)) {
    __SET_VAR(data__->,ENO,,__BOOL_LITERAL(FALSE));
    return;
  }
  else {
    __SET_VAR(data__->,ENO,,__BOOL_LITERAL(TRUE));
  }
  // Initialise TEMP variables

  if ((__GET_VAR(data__->TRIG,) && !(__GET_VAR(data__->TRIG0,)))) {
    #define GetFbVar(var,...) __GET_VAR(data__->var,__VA_ARGS__)
    #define SetFbVar(var,val,...) __SET_VAR(data__->,var,__VA_ARGS__,val)

   LogMessage(GetFbVar(LEVEL),(char*)GetFbVar(MSG, .body),GetFbVar(MSG, .len));
  
    #undef GetFbVar
    #undef SetFbVar
;
  };
  __SET_VAR(data__->,TRIG0,,__GET_VAR(data__->TRIG,));

  goto __end;

__end:
  return;
} // LOGGER_body__() 





void RS485_MASTER_init__(RS485_MASTER *data__, BOOL retain) {
  __INIT_VAR(data__->PB3,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->LED3,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->PB2,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->LED2,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->PB1,0,retain)
  __INIT_VAR(data__->LED1,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->_TMP_GT11_OUT,__BOOL_LITERAL(FALSE),retain)
}

// Code part
void RS485_MASTER_body__(RS485_MASTER *data__) {
  // Initialise TEMP variables

  __SET_VAR(data__->,LED3,,__GET_VAR(data__->PB3,));
  __SET_VAR(data__->,LED2,,__GET_VAR(data__->PB2,));
  __SET_VAR(data__->,_TMP_GT11_OUT,,GT__BOOL__WORD(
    (BOOL)__BOOL_LITERAL(TRUE),
    NULL,
    (UINT)2,
    (WORD)__GET_VAR(data__->PB1,),
    (WORD)240));
  __SET_VAR(data__->,LED1,,__GET_VAR(data__->_TMP_GT11_OUT,));

  goto __end;

__end:
  return;
} // RS485_MASTER_body__() 





