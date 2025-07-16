int myScanf(char* inBuf, char* argBuf1, char* argBuf2,  char* argBuf3)
{
  int i=0;

  argBuf1[0]='\0';
  argBuf2[0]='\0';
  argBuf3[0]='\0';

  if((i=myGetArg(inBuf, i, argBuf1))>0){
    if((i=myGetArg(inBuf, i, argBuf2))>0){
      return myGetArg(inBuf, i, argBuf3);
    }else{
      return i;
    }
  }else{
    return i;
  }
  return i;
}

int myGetArg(char* inBuf, int i, char* argBuf){
  int j;

  while(i<MAX_LINE_LENGTH){
    if(isblank(inBuf[i])){
      i++;
    }else if(iscntrl(inBuf[i])){
      return 0;
    }else {
      break;
    }
  }

  for(j=0;j<MAX_PARAM_LENGTH;j++){
    if(i<MAX_LINE_LENGTH){
      if(isblank(inBuf[i])){
	argBuf[j]='\0';

      }else if(iscntrl(inBuf[i])){
	argBuf[j]='\0';
	return 0;
      }else{
	argBuf[j]=inBuf[i];
	i++;
      }
    }else{
      return -1;
    }
  }
  return i;
}
