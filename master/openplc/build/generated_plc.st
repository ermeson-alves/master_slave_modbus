PROGRAM RS485_Master
  VAR_INPUT
    Start_Button : BOOL;
    Motor_Enable : BOOL;
  END_VAR

  Motor_Enable := Start_Button;
END_PROGRAM


CONFIGURATION Config0

  RESOURCE Res0 ON PLC
    TASK task0(INTERVAL := T#20ms,PRIORITY := 0);
    PROGRAM instance0 WITH task0 : RS485_Master;
  END_RESOURCE
END_CONFIGURATION
