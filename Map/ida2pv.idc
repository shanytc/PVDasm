////////////////////////////////////////////
///      IDA 2 PVDasm Analysis Converter ///
///                                      ///
///           By Bengaly/2005            ///
///        For PVdasm 1.5d and Above     ///
////////////////////////////////////////////


///////////////- Notes: -///////////////////
// Run this Script After IDA has Finished //
// It's File Analysis.                    //
// The script will create a .MAP file     //
// To be used with PVDasm in 'c:\'        //
////////////////////////////////////////////

#include <idc.idc>

//
// get real end addr of the funtion
//
static GetEndOfFunc(start,addr)
{
  auto last_addr,temp,i,count,backward;
  
  if(start==addr)
    return -1;

  if(addr<start)
    return -1;

  count=0;
  backward=start;
  temp=backward; 
  
  for(i=0;i<(addr-start);i++){
    temp=FindCode(temp,SEARCH_DOWN);
    if(temp>=addr)          
      break;

    count++;     
  }

  last_addr=backward;
  for(i=0;i<count;i++)
     last_addr=FindCode(last_addr,SEARCH_DOWN);

  return last_addr;
}

// this functions returns the number of subroutines
// includes the 'main', and exludes the 'jmp api' addresses.
static GetNumFunctions(){
 auto i,x,y,str,ea;

 i=0;
 for ( ea=NextFunction(0); ea != BADADDR; ea=NextFunction(ea) ) 
 {    
   i++;
 }
  
  return i;
}

static FuncLength(start,end)
{
 auto len;

 if(start==end)
   return 0;

 if(end<start) 
   return 0;

 len=end-start;
 return len;
}

//
// Saves addresses of all functions (sub/ep)
//
static ExportFunctions(filename,total){
  auto ea,x,str,y,file,end_addr,length;

  if(total==0)
  {
    Message("No Functions to export!");
    return -1;
  }

  file=fopen(filename,"a+");
  str=form("[FUNCTIONS]\n");
  fprintf(file,str);
  total=0;

  for ( ea=NextFunction(0); ea != BADADDR; ea=NextFunction(ea) ) 
  {  
      end_addr=GetEndOfFunc(ea,FindFuncEnd(ea));
      if(end_addr!=-1)
      {
        length=FuncLength(ea,end_addr);
        if(length>0)
        {
          total++;
          str=form("%08lX:%08lX:%s",ea,end_addr,GetFunctionName(ea));
          str=str+"\n";        
          fprintf(file,str);
          //str=form("%08lX:%08lX ; %s ; len=%d bytes",ea,end_addr,GetFunctionName(ea),length); // remove this
          //Message(str);        
          //Message("\n");
        }
      }
  }

  str=form("Total=%d\n",total);
  fprintf(file,str);

  fclose(file);
}

static ExportData(filename){
  auto ea,total,file,addr,str,end;

  file=fopen(filename,"a+");
  str=form("[DATA]\n");
  fprintf(file,str);

  total=0;

  for ( ea=NextFunction(0); ea != BADADDR; ea=NextFunction(ea) ) 
  {
     addr=ea;
     do{
       addr=FindData(addr,SEARCH_DOWN); 
       end=FindCode(addr,SEARCH_DOWN);
       
       if(end!=BADADDR && addr!=BADADDR)
       {
        total++;
        str=form("%08lX:%08lX\n",addr,end-1);
        //Message(str);
        fprintf(file,str);
       }
       addr=end;     
    }while(end!=BADADDR && addr!=BADADDR);

  }

  str=form("Total=%d\n",total);
  fprintf(file,str);

  fclose(file);

}

static main() {
  auto FileName;
  FileName=form("C:\\%s.map",GetInputFile());
  Message("\n***** IDA To PVDasm MAP Creator by Ben 5.9.2004 v1.0 *****\n");
  Message(FileName);
  Message("\n\nExporting Functions...\n");
  ExportFunctions(FileName,GetNumFunctions());

  Message("\nExporting Data Blocks...\n");
  ExportData(FileName);

  Message("\n\nFile Created.");
}
