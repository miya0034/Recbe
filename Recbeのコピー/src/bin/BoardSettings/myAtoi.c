unsigned int myAtoi(char* str){
  char temp[256];
  int i=0, j=0, value=0, endProc=0;
  strcpy(temp,str);

  if((temp[i]=='0')||isblank(temp[i])) i++;
  if((temp[i]=='x')||(temp[i]=='X')){
    i++;
    j++;
    while((endProc==0)&&(temp[i]!='\0')){
      if(isdigit(temp[i])){
	value=value*16;
	value+=temp[i]-'0';
	i++;
      }else if(isxdigit(temp[i])){
	value=value*16;

	if(isupper(temp[i])){
	  value+=temp[i]-'A'+10;
	}else{
	  value+=temp[i]-'a'+10;
	}
	i++;
      }else{
	endProc=1;
	//	puts("endProc");
      }
    }
    if(j>8){
      puts("Error: too large value is detected.");
      return 0xFFFFFFFF;
    }
  }else{
    while(isdigit(temp[i])){
      value=value*10;
      value+=temp[i]-'0';
      i++;
      j++;
      if(j>10){
	puts("Error: too large value is detected.");
	return 0xFFFFFFFF;
      }
    }
  }

  return value;
}
