#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>


using namespace std;

long int access_Time = 0;
long int load_Time =0;


class Line
{
    public:
    string tag=NULL;
    int dirty;
    int lru_bit;
    long int lru_counter;
    long int fifo_counter;
    Line()
    {
        dirty=0;
        lru_bit=0;
    }
};

class Cache
{
    public:
    int cacheSize;
    int associativity;
    int replacementPolicy;
    int writePolicy;
    int blockSize;
    int incPolicy;
    int flag;

    Line * lines;

    void setLines(int cacheSize, int associativity, int replacementPolicy, int writePolicy, int blockSize, int incPolicy )
    {
        this->cacheSize = cacheSize;
        this->associativity = associativity;
        this->replacementPolicy = replacementPolicy;
        this->writePolicy = writePolicy;
        this->blockSize = blockSize;
        this->incPolicy = incPolicy;

        int numLines = cacheSize/blockSize;

        lines = (Line *)malloc(numLines * sizeof(Line));
    }
    void showDetails()
    {
        cout<<"Cache Size : "<<cacheSize<<endl;
        cout<<"Block Size : "<<blockSize<<endl;

    }

};

class Cache_Hier
{
    public:

    int numLevels;
    int incPolicy; //0-LRU 1-FIFO 2-SRRIP 3-Pseudo LRU
    Cache * caches;

    void setCaches(int numLevels)
    {
        this->numLevels = numLevels;
        caches = (Cache *)malloc(numLevels * sizeof(Cache));
    }
    void showDetails()
    {
        cout<<numLevels;
    }

};

class Statistics
{
    public:
    int mem_Refs;
    int num_Reads;
    int num_Writes;
    int num_Hits;
    int num_Misses;//not to be written
    double miss_Ratio;
    int num_Write_Backs;
    int num_Clean_Evicts;
    Statistics()
    {
        mem_Refs=0;
        num_Reads=0;
        num_Writes=0;
        num_Hits=0;
        num_Misses=0;
        miss_Ratio=0;
        num_Write_Backs=0;
        num_Clean_Evicts=0;
    }

};

struct Tag_Index
{
    string tag;
    string index;
};

Tag_Index find_tag_index(string bin_address, int blockSize, int cacheSize, int associativity)
{

    int sets = cacheSize/(blockSize*associativity);
    int index = log2(sets);

    int offset = log2(blockSize);
    string tagg;
    string indexx;


    int tag = 64 - index - offset;

    tagg= bin_address.substr(0,tag);
    indexx= bin_address.substr(tag+1,index);
    
    Tag_Index output;
    output.tag = tagg;
    output.index = indexx;

    return output;
  
}

string hex_char_to_bin(char c)
{
    string s;
    switch(toupper(c))
    {
        case '0': { s= "0000"; return s;}
        case '1': { s= "0001"; return s;}
        case '2': { s= "0010"; return s;}
        case '3': { s= "0011"; return s;}
        case '4': { s= "0100"; return s;}
        case '5': { s= "0101"; return s;}
        case '6': { s= "0110"; return s;}
        case '7': { s= "0111"; return s;}
        case '8': { s= "1000"; return s;}
        case '9': { s= "1001"; return s;}
        case 'A': { s= "1010"; return s;}
        case 'B': { s= "1011"; return s;}
        case 'C': { s= "1100"; return s;}
        case 'D': { s= "1101"; return s;}
        case 'E': { s= "1110"; return s;}
        case 'F': { s= "1111"; return s;}
    }
}

string hex_str_to_bin_str(string hex)
{
    
    string bin;
    for(unsigned i = 0; i != hex.length(); ++i)
       bin += hex_char_to_bin(hex[i]);
    return bin;
}

long int bin2decimal(string bin)
{
    int bin_size;
    bin_size = bin.size();
    int j= bin_size-1;
    long int decimal=0;
    for(int i=0;i<bin_size;i++){
        if(bin[i] == '1')
        {
            decimal = decimal+pow(2,j);
        }
        j=j-1;
    }
    return decimal;
}

int getTagPos (Cache cache, string index, string tag) 
{
    int block_i;
    
    long int start = bin2decimal(index) * cache.associativity;
    long int end = start + cache.associativity -1;

    for (block_i=start; block_i<=end; block_i++) 
    {
        if (tag.compare(cache.lines[block_i].tag) == 0) 
        {
            return block_i; //tag found in the set -> hit
        }
    }
    return -1; //tag not found in set -> miss
}

int checkFull(Cache cache, string index)
{
    int block_i;
    long int start = bin2decimal(index) * cache.associativity;
    long int end = start + cache.associativity -1;


    for (block_i=start; block_i<=end; block_i++) 
    {
        if (cache.lines[block_i].tag.compare("") == 0) 
            return -1 ; // if atleast 1 tag is null then set is not full
    }
    return 1;
}

int get_First_Free_Pos(Cache cache, string index)
{
    int block_i;
    long int start = bin2decimal(index) * cache.associativity;
    long int end = start + cache.associativity -1;


    for (block_i=start; block_i<=end; block_i++) 
    {
        if (cache.lines[block_i].tag.compare("") == 0) 
        {
            return block_i; //return the first position where tag is null
        }
    }
    return start;
}

int findLRUBlock (Cache cache, string index) 
{
    int block_i; //to iterate over all blocks in set
    int LRUBlock=0; //victim block
    long int start = bin2decimal(index) * cache.associativity;
    long int end = start + cache.associativity -1;

    long unsigned leastAccess = cache.lines[start].lru_counter;// least recently accessed block
    for (block_i=start+1; block_i<=end; block_i++) {
        if (cache.lines[block_i].lru_counter < leastAccess) 
        {
            leastAccess = cache.lines[block_i].lru_counter;
            LRUBlock = block_i;
        }
    }
    return LRUBlock;
}

int findPseudoLRUBlock (Cache cache, string index, int hit, int lineNo) 
{
    int block_i; //to iterate over all blocks in set
    int pseudoLRUBlock=lineNo; //victim block
    long int start = bin2decimal(index) * cache.associativity;
    long int end = start + cache.associativity -1;
    int flag=0;

    //long unsigned leastAccess = cache.lines[start].lru_bit;// least recently accessed block
    if(hit==0)
    {
        for (block_i=start; block_i<=end; block_i++) 
        {
            if (cache.lines[block_i].lru_bit==0) 
            {
                //cache.lines[block_i].lru_bit=1;
                pseudoLRUBlock = block_i;
                break;
            }
        }
    }
    cache.lines[pseudoLRUBlock].lru_bit=1;
    for (block_i=start; block_i<=end; block_i++) 
        {
            if (cache.lines[block_i].lru_bit==0) 
            {
                flag=1;
                break;
            }
        }


        if(flag==0)
        {
           for (block_i=start; block_i<=end; block_i++) 
            {
                if (block_i!=pseudoLRUBlock) 
                {
                    cache.lines[block_i].lru_bit=0;
                }
            } 
        }
    
   
    return pseudoLRUBlock;
}

int findFIFOBlock (Cache cache, string index) 
{
    int block_i; //to iterate over all blocks in set
    int FIFOBlock=0; //victim block
    long int start = bin2decimal(index) * cache.associativity;
    long int end = start + cache.associativity -1;

    long unsigned leastLoad = cache.lines[start].fifo_counter;// least recently Loaded block
    for (block_i=start+1; block_i<=end; block_i++) {
        if (cache.lines[block_i].fifo_counter < leastLoad) 
        {
            leastLoad = cache.lines[block_i].lru_counter;
            FIFOBlock = block_i;
        }
    }
    return FIFOBlock;
}

void updateUpperLevel(string address, Cache_Hier cHier, int level, Statistics * result)
{


    if(level < cHier.numLevels)
    {
        result[level].num_Writes++;
        Tag_Index tI = find_tag_index(address,cHier.caches[level].blockSize,cHier.caches[level].cacheSize,cHier.caches[level].associativity);
        
        int tag_position = getTagPos(cHier.caches[level],tI.index, tI.tag);

        if(cHier.caches[level].writePolicy==1)
        {
            cHier.caches[level].lines[bin2decimal(tI.index)].dirty = 1;
        }

        else
        {
            result[level].num_Write_Backs++;
            updateUpperLevel(address,cHier,level+1,result);
        }

    }
    
}

void evictFromLowerLevels(string address, Cache_Hier cHier, int level, Statistics * result)
{
    //cout << "Entered evict from lover levels" << endl;
    //cout << address << endl;
    Tag_Index tI = find_tag_index(address,cHier.caches[level].blockSize,cHier.caches[level].cacheSize,cHier.caches[level].associativity);
    //cout << "find tag index crossed" << endl;
    //cout << "tag index" << tI.index << endl;
    //cout << "tag " << tI.tag << endl;
    
    int tag_position = getTagPos(cHier.caches[level],tI.index, tI.tag);
    //cout << "crossed get tagpos" << endl;
    if(tag_position!=-1)//block is present in lower level cache
    {
        // cout << "before bin2decimal" << endl;
        // cout << bin2decimal(tI.index) << endl;
        // cout << level << ":: level" << endl;
        // cout << "line exist:" << cHier.caches[0].lines[14].tag << endl;
        //if the replac policy is write back and the block is dirty we have to write the block in all higher levels
        //cHier.caches[level].lines[bin2decimal(tI.index)].tag ="akamd";
        //cout << "crossed bn2decimal" << endl;
        
        //cout << "line exist:" << cHier.caches[level].lines[bin2decimal(tI.index)] << endl;
        if((cHier.caches[level].writePolicy==1)&&(cHier.caches[level].lines[bin2decimal(tI.index)].dirty == 1))
        {
             //if level is first check for dirty bit and increment memwriteback
            updateUpperLevel(address,cHier,level+1,result);
            cHier.caches[level].lines[bin2decimal(tI.index)].dirty = 0;
            result[level].num_Write_Backs++;
        }
        else
        {
            result[level].num_Clean_Evicts++;
            if(level!=0)
            evictFromLowerLevels(address,cHier,level-1,result);
        }
       
    }

    //cout << "left evict evictFromLowerLevels" << endl;
    
}

struct Tag_Index ex_pseudo_lru(struct Tag_Index address, Cache_Hier cHier ,  int level,Statistics *result ,int hit,int hit_linenum )
{
    Tag_Index next_pass_Address;
    int associativity;
    associativity = cHier.caches[level].associativity;
    long int start_line;
    long int end_line;
    start_line = associativity*bin2decimal(address.index);
    end_line = start_line+ associativity -1;
    
    int first_line_lru;
    first_line_lru = hit_linenum;
    int flag=0;
    if(hit == 0)
    {
        for (long int block_i=start_line; block_i<=end_line; block_i++) 
    {
        if (cHier.caches[level].lines[block_i].lru_bit == 0){
            first_line_lru = block_i;
            break;
        }
                      // Upper inside of the set found
    }
}

    cHier.caches[level].lines[first_line_lru].lru_bit = 1;
    
    

    for (long int block_i=start_line; block_i<=end_line; block_i++) 
    {
        if (cHier.caches[level].lines[block_i].lru_bit == 0){
            flag = 1;
            break;
        }
                      // Upper inside of the set found
    }
    //cout << "clear till here" << endl;
    if(flag == 0)
    {
            //cout << "clear till here" << endl;

        for (int block_i=start_line; block_i<=end_line; block_i++) 
    {
        //cout << "hi" << endl;
        if (block_i!=first_line_lru)
        {
            cHier.caches[level].lines[block_i].lru_bit =0;
            
        }
                      // Upper inside of the set found
    }
        //cout << "clear till here" << endl;


    }
    //cHier.caches[level].lines[replace_linenum].lru_counter  = access_Time;

    cHier.caches[level].lines[first_line_lru].tag = address.tag;
    if(cHier.caches[level].lines[first_line_lru].dirty == 0)
    {
        result[level].num_Clean_Evicts++;

    }

    //cout << "clear here" << endl;


    next_pass_Address.tag = cHier.caches[level].lines[first_line_lru].tag;
    next_pass_Address.index = address.index;
        //cout << "clear here" << endl;

    string add = next_pass_Address.tag + next_pass_Address.index;
       // cout << "clear here" << endl;

if(cHier.numLevels != level+1)
{
    next_pass_Address = find_tag_index(add,cHier.caches[level+1].blockSize,cHier.caches[level+1].cacheSize, cHier.caches[level+1].associativity);
    
}
return next_pass_Address;
    


}

struct Tag_Index ex_fifo(struct Tag_Index address, Cache_Hier cHier ,int level,Statistics *result )
{
    // cout << "entered lru" << endl;
    Tag_Index next_pass_Address;
    int associativity;
    associativity = cHier.caches[level].associativity;
    long int start_line;
    long int end_line;
    start_line = associativity*bin2decimal(address.index);
    end_line = start_line+ associativity -1;
    long int replace;
    long int replace_linenum;
    replace  = cHier.caches[level].lines[start_line].fifo_counter ;
    replace_linenum = start_line;
    // cout << "crossed all declarations" << endl;
    for (long int block_i=start_line; block_i<=end_line; block_i++) 
    {
        // cout << "enterd for loop" << endl;
        if (replace > cHier.caches[level].lines[block_i].fifo_counter ){
            replace =  cHier.caches[level].lines[block_i].fifo_counter; 
            replace_linenum = block_i; 
        }
                      // Upper inside of the set found
    }
    // cout << "crossed lru for loop" << endl;

    cHier.caches[level].lines[replace_linenum].tag = address.tag;
    if(cHier.caches[level].lines[replace_linenum].dirty == 0){
        result[level].num_Clean_Evicts++;

    }

    // cout << "coressed if after lru for loop" << endl;



    next_pass_Address.tag = cHier.caches[level].lines[replace_linenum].tag;
    next_pass_Address.index = address.index;
    // cout << " crossed intilization next pass address" << endl;
    string add = next_pass_Address.tag + next_pass_Address.index;
    next_pass_Address = find_tag_index(add,cHier.caches[level+1].blockSize,cHier.caches[level+1].cacheSize, cHier.caches[level+1].associativity);
    // cout << "crossed finding tag and index" << endl;
    // cout << next_pass_Address.tag << endl;
    // cout << next_pass_Address.index << endl;
    return next_pass_Address;
}


struct Tag_Index LRU(struct Tag_Index address, Cache_Hier cHier ,  int level,Statistics *result )
{
    // cout << "entered lru" << endl;
    Tag_Index next_pass_Address;
    int associativity;
    associativity = cHier.caches[level].associativity;
    long int start_line;
    long int end_line;
    start_line = associativity*bin2decimal(address.index);
    end_line = start_line+ associativity -1;
    long int replace;
    long int replace_linenum;
    replace  = cHier.caches[level].lines[start_line].lru_counter ;
    replace_linenum = start_line;
    // cout << "crossed all declarations" << endl;
    for (long int block_i=start_line; block_i<=end_line; block_i++) 
    {
        // cout << "enterd for loop" << endl;
        if (replace > cHier.caches[level].lines[block_i].lru_counter ){
            replace =  cHier.caches[level].lines[block_i].lru_counter; 
            replace_linenum = block_i; 
        }
                      // Upper inside of the set found
    }
    // cout << "crossed lru for loop" << endl;

    cHier.caches[level].lines[replace_linenum].lru_counter  = access_Time;
    cHier.caches[level].lines[replace_linenum].tag = address.tag;
    if(cHier.caches[level].lines[replace_linenum].dirty == 0){
        result[level].num_Clean_Evicts++;

    }

    // cout << "coressed if after lru for loop" << endl;



    next_pass_Address.tag = cHier.caches[level].lines[replace_linenum].tag;
    next_pass_Address.index = address.index;
    // cout << " crossed intilization next pass address" << endl;
    string add = next_pass_Address.tag + next_pass_Address.index;
    next_pass_Address = find_tag_index(add,cHier.caches[level+1].blockSize,cHier.caches[level+1].cacheSize, cHier.caches[level+1].associativity);
    // cout << "crossed finding tag and index" << endl;
    // cout << next_pass_Address.tag << endl;
    // cout << next_pass_Address.index << endl;
    return next_pass_Address;
}


void ex_read_cache (Cache_Hier cHier, string address,Statistics *result,int rw) {
    /** Reading the data in ute set (by index) in the position that contains the
      *     upper (by line).
      */

    int numLevels = cHier.numLevels;
    int found_tag_position;
    string hex_addr_string = hex_str_to_bin_str(address);
    int found=0;
    struct Tag_Index* prev_tag;
    Tag_Index tInd;
    Tag_Index random;
    int cache_index;
    int i;
    int counter=0;
    //cout << "declareation" << endl;
    for(i=0; i<numLevels; i++)
    {
        result[i].mem_Refs++;
        result[i].num_Reads++;
      //  cout << "inside for loops" << endl;
        tInd = find_tag_index(hex_addr_string,cHier.caches[i].blockSize,cHier.caches[i].cacheSize,cHier.caches[i].associativity);
       // cout << "tag is::" << tInd.tag << endl;
        // cout << "Index is::" << tInd.index << endl;

        found_tag_position = getTagPos(cHier.caches[i],tInd.index,tInd.tag);

        if(found_tag_position == -1)
        {
            result[i].num_Misses++;

            
        }
        else{
            found=1;
           // cout << "psrandin" << endl;
            if(cHier.caches[cache_index].replacementPolicy == 3)
            {
                random = ex_pseudo_lru(tInd,cHier ,i,result ,1,found_tag_position );
            }

            //random = ex_pseudo_lru(tInd,cHier ,i,result ,1,found_tag_position );

            cHier.caches[i].lines[i].lru_counter  = access_Time;

            cHier.caches[i].lines[found_tag_position].lru_bit =1;

            result[i].num_Hits++;
            break;
        }

         //cout << "inside the function" << endl;

    }
    //continue;
    // cout << "crossed for loop" << endl;

    if(i>0 && found ==1){
        // cout << "i>0 and found starting" << endl;
        cHier.caches[i].lines[found_tag_position].tag = "";
        // cout << "i>0 and found ending" << endl;

        // eviction++;
        
    }
    else if(i==0 && found ==1){

        if(rw==1){
            // cout << "i=0 and found starting" << endl;
            cHier.caches[i].lines[found_tag_position].dirty = 1;
            result[0].num_Writes++;
            result[0].mem_Refs++;
            // cout << "i=0 and found ending" << endl;
        }
        
    }

    else if(i!=0){
        // cout<<"Entering i!=0"<<endl;
        cache_index = 0;
  //prev_tag =  cHier.caches[0].lines[found_tag_position].tag;
  //cHier.caches[0].lines[found_tag_position].tag = tInd.tag;
    int flag_to_next_level=1;
    int call_replacement_algo=0;
    int check_cache_full;
    int free_line_num;
    Tag_Index eviction_tag;

    eviction_tag = tInd;
  while(flag_to_next_level){
    //cout<<"Entered while loop"<<endl;
    //cout << cache_index << endl;
    // cout<<cache_index<<endl<<eviction_tag.index<<endl;
     tInd = find_tag_index(hex_addr_string,cHier.caches[cache_index].blockSize,cHier.caches[cache_index].cacheSize,cHier.caches[cache_index].associativity);
     //cout << cache_index << endl;
      eviction_tag = tInd;
         check_cache_full = checkFull(cHier.caches[cache_index], eviction_tag.index);
         
         free_line_num = get_First_Free_Pos(cHier.caches[cache_index], eviction_tag.index); // updated

         cHier.caches[cache_index].lines[free_line_num].lru_counter  = access_Time; // updaed
         cHier.caches[cache_index].lines[free_line_num].fifo_counter  = access_Time;
         result[cache_index].mem_Refs++;
      //   cout<<"Vamsi"<<endl;
         // cout<<"Full is checked"<<endl;
         if(check_cache_full == -1)
         {
            // cout << "index" << eviction_tag.index << endl;
            
            // cout << "after get free positon" << endl;
            // cout << "cache_index" << cache_index << endl;
        //    cout << "free line num" << free_line_num << endl;
            if(cHier.caches[cache_index].replacementPolicy == 3)
            { 
          //      cout << "if" << endl;
                random = ex_pseudo_lru(tInd,cHier ,cache_index,result ,0,0 );
            }
            else{
            //    cout << "else" << endl;
                cHier.caches[cache_index].lines[free_line_num].tag = eviction_tag.tag;
            }
            
            //cout << "after intilization of tag" << endl;

            
            flag_to_next_level = 0;
            //eviction_tag = LRU(eviction_tag,cHier , cache_index );

         }
         else
         {

            result[cache_index+1].num_Writes++;
            //result[cache_index+1].mem_Refs++;
            if(cHier.caches[cache_index].replacementPolicy == 0)
            {
                 eviction_tag = LRU(eviction_tag,cHier , cache_index ,result);
              //   cout << "LRU" << endl;

            }
            else if(cHier.caches[cache_index].replacementPolicy == 1)
            {
                eviction_tag = ex_fifo(eviction_tag,cHier , cache_index ,result); 

                //cout << "FIFO" << endl;

            }
            else if(cHier.caches[cache_index].replacementPolicy == 3)
            {
                eviction_tag = ex_pseudo_lru(tInd,cHier ,cache_index,result ,0,0 );
                //cout << "pslru" << endl;
            }
           
           
            


            result[cache_index].num_Write_Backs+=1;
            
            cache_index +=1;
            if(cache_index == numLevels)
            {
                result[cache_index-1].num_Write_Backs++;
                flag_to_next_level = 0;
            }


        }
        

  }
    // prev_tag = ex_read_cache_helper(Cache_Hier cHier,Statistics *result,cache_index,tInd);
  


    }
    

}


void read_cache (Cache_Hier cHier, string address,Statistics *result) 
{

    int numLevels = cHier.numLevels;
    string hex_addr_string = hex_str_to_bin_str(address);
    int level_found=1, found=0;
    Tag_Index tInd[numLevels];
    int found_tag_position;
    

    for(int i=0; i<numLevels; i++)
    {
        result[i].mem_Refs++;
        result[i].num_Reads++;
        tInd[i] = find_tag_index(hex_addr_string,cHier.caches[i].blockSize,cHier.caches[i].cacheSize,cHier.caches[i].associativity);
        found_tag_position = getTagPos(cHier.caches[i],tInd[i].index,tInd[i].tag);

        //cout<<"Found tag at : "<<found_tag_position<<endl;
        if(found_tag_position !=-1)
        {
            cHier.caches[i].lines[bin2decimal(tInd[i].index)].lru_counter = access_Time;
            if(cHier.caches[i].replacementPolicy==3)
            {
              int random =  findPseudoLRUBlock(cHier.caches[i],tInd[i].index,1,bin2decimal(tInd[i].index));
            }
            result[i].num_Hits++;
            found = 1;
            break;
        }

        else
        {
            result[i].num_Misses++;
            level_found++;
        }
    }

    if(level_found!=1)
    {
        //cout << "entered level gound"<< endl;
        while(--level_found)
        {
           // cout << "entered while lebel found" << endl;
            int is_set_full = checkFull(cHier.caches[level_found-1],tInd[level_found-1].index);
            //cout << "coressed check Full" << endl;
            if(is_set_full == 1)
            {
                //cout << "enterd is set full " << endl;
                //apply lru replc. also evict from lower levels after checking dirty bit
                int repBlock;
                if(cHier.caches[level_found-1].replacementPolicy==0 || cHier.caches[level_found-1].replacementPolicy==2)
                repBlock = findLRUBlock(cHier.caches[level_found-1],tInd[level_found-1].index);
                else if(cHier.caches[level_found-1].replacementPolicy==1)
                repBlock = findFIFOBlock(cHier.caches[level_found-1],tInd[level_found-1].index);
                else if(cHier.caches[level_found-1].replacementPolicy==3)
                repBlock = findPseudoLRUBlock(cHier.caches[level_found-1],tInd[level_found-1].index,0,bin2decimal(tInd[level_found-1].index));

                 //cout << "found lru block" << endl;
                 string address = cHier.caches[level_found-1].lines[repBlock].tag + tInd[level_found-1].index; //+'\0'
                 evictFromLowerLevels(address, cHier, level_found-1, result);
                 //cout << "evictFromLowerLevels  crossed "<< endl;
                 cHier.caches[level_found-1].lines[repBlock].tag = tInd[level_found-1].tag;
                 cHier.caches[level_found-1].lines[repBlock].fifo_counter = load_Time;
                 cHier.caches[level_found-1].lines[repBlock].lru_counter = access_Time;
            }
            else
            {
                int free_pos = get_First_Free_Pos(cHier.caches[level_found-1],tInd[level_found-1].index);
                cHier.caches[level_found-1].lines[free_pos].tag = tInd[level_found-1].tag;
                 cHier.caches[level_found-1].lines[free_pos].fifo_counter = load_Time;
                cHier.caches[level_found-1].lines[free_pos].lru_counter = access_Time;

            }

        }
    }
        
}

void write_cache (Cache_Hier cHier,string address,Statistics *result)
{
    int numLevels = cHier.numLevels;
    string hex_addr_string = hex_str_to_bin_str(address);
    int level_found=1, found=0;
    Tag_Index tInd[numLevels];
    int found_tag_position;
     for(int i=0; i<numLevels; i++)
    {
        result[i].mem_Refs++;
        //result[i].num_Writes++;
        tInd[i] = find_tag_index(hex_addr_string,cHier.caches[i].blockSize,cHier.caches[i].cacheSize,cHier.caches[i].associativity);
        found_tag_position = getTagPos(cHier.caches[i],tInd[i].index,tInd[i].tag);
        if(found_tag_position !=-1)
        {
            cHier.caches[i].lines[bin2decimal(tInd[i].index)].lru_counter = access_Time;
            result[i].num_Hits++;
            found = 1;
            break;
        }

        else
        {
            result[i].num_Misses++;
            level_found++;
        }
    }

    if(level_found==1)
    {
        cHier.caches[0].lines[bin2decimal(tInd[0].index)].tag = tInd[0].tag;
        cHier.caches[0].lines[bin2decimal(tInd[0].index)].lru_counter = access_Time;
        result[0].num_Writes++;
        if(cHier.caches[0].writePolicy==1)
        {
            cHier.caches[0].lines[bin2decimal(tInd[0].index)].dirty = 1;

        }
        else
        {
            result[0].num_Write_Backs++;
            updateUpperLevel(tInd[0].tag+tInd[0].index,cHier,level_found,result);
            
            //update on all higher levels of chache and memory
        }
    }

    else if(level_found!=1)
    {
        while(--level_found)
        {
            int is_set_full = checkFull(cHier.caches[level_found-1],tInd[level_found-1].index);
            if(is_set_full == 1)
            {
                //apply lru replc. also evict from lower levels after checking dirty bit
                int repBlock;
                if(cHier.caches[level_found-1].replacementPolicy==0 || cHier.caches[level_found-1].replacementPolicy==2)
                repBlock = findLRUBlock(cHier.caches[level_found-1],tInd[level_found-1].index);
                else if(cHier.caches[level_found-1].replacementPolicy==1)
                repBlock = findFIFOBlock(cHier.caches[level_found-1],tInd[level_found-1].index);
                else if(cHier.caches[level_found-1].replacementPolicy==3)
                repBlock = findPseudoLRUBlock(cHier.caches[level_found-1],tInd[level_found-1].index,0,bin2decimal(tInd[level_found-1].index));

                 string address = cHier.caches[level_found-1].lines[repBlock].tag + tInd[level_found-1].index; //+'\0'
                 evictFromLowerLevels(address, cHier, level_found-1, result);
                 cHier.caches[level_found-1].lines[repBlock].tag = tInd[level_found-1].tag;
                 cHier.caches[level_found-1].lines[repBlock].fifo_counter = load_Time;
                 cHier.caches[level_found-1].lines[repBlock].lru_counter = access_Time;
            }
            else
            {
                int free_pos = get_First_Free_Pos(cHier.caches[level_found-1],tInd[level_found-1].index);
                cHier.caches[level_found-1].lines[free_pos].tag = tInd[level_found-1].tag;
                cHier.caches[level_found-1].lines[free_pos].fifo_counter = load_Time;
                cHier.caches[level_found-1].lines[free_pos].lru_counter = access_Time;

            }

        }

        if(cHier.caches[0].writePolicy==1)
        {
            
            cHier.caches[0].lines[bin2decimal(tInd[0].index)].dirty = 1;

        }
        else
        {
            result[0].num_Write_Backs++;
             updateUpperLevel(tInd[0].tag+tInd[0].index,cHier,1,result);
              
            //update on all higher levels of chache and memory
        }
    }
}


int main(int argc, char ** argv)
{
    int numLevels, incPolicy, blockSize, cacheSize, associativity, replacementPolicy, writePolicy;
    Cache_Hier cHier;
    char address[17];
    int rw;
    string tag, index;
    Tag_Index obj2;


    char * specs = argv[1];
    char * traces = argv[2];

    FILE * specsFile = fopen(specs,"rb");
    FILE * tracesFile;
    FILE * statsFile;

    fscanf(specsFile, "%d %d %d\n", &numLevels, &incPolicy, &blockSize);

    
    cHier.setCaches(numLevels);

    Statistics out_Obj[numLevels];

    for(int i=0; i<numLevels; i++)
    {
        fscanf(specsFile, "%d %d %d %d\n", &cacheSize, &associativity, &replacementPolicy, &writePolicy);
        cHier.caches[i].setLines(cacheSize,associativity,replacementPolicy,writePolicy,blockSize,incPolicy);
    }

    fclose(specsFile);
   
    tracesFile = fopen(traces, "rb");
    if(incPolicy==0)
    {
        while (fscanf(tracesFile, "0x%s %d\n", address, &rw) != EOF)
        {
            
            access_Time++;
            load_Time++;
            if(rw == 0)

            {
                //cout << "read"<< endl;
                read_cache(cHier,address,out_Obj);
            }

            else if(rw == 1)
            {
                //cout << "write" << endl;
               write_cache(cHier,address,out_Obj);
            }

            
        }

    }

    else
    {
        while(fscanf(tracesFile, "0x%s %d\n", address, &rw) != EOF)
        {
            ex_read_cache(cHier,address,out_Obj,rw);
        }
    }
    

    fclose(tracesFile);

    statsFile = fopen("Stats.txt","wb");
    long double missRatio;
    for(int i=0;i<numLevels;i++)
    {
        missRatio = out_Obj[i].num_Misses*1.0/out_Obj[i].mem_Refs;
        fprintf(statsFile, "C0.L%d.memRefs = %d\n",i,out_Obj[i].mem_Refs);
        fprintf(statsFile, "C0.L%d.numReads = %d\n",i,out_Obj[i].num_Reads);
        fprintf(statsFile, "C0.L%d.numWrites= %d\n",i,out_Obj[i].num_Writes);
        fprintf(statsFile, "C0.L%d.numHits= %d\n" ,i,out_Obj[i].num_Hits);
        fprintf(statsFile, "C0.L%d.missRatio= %Lf\n",i,missRatio);
        fprintf(statsFile, "C0.L%d.numWritebacks= %d\n",i,out_Obj[i].num_Write_Backs);
        fprintf(statsFile, "C0.L%d.numCleanEvicts= %d\n",i,out_Obj[i].num_Clean_Evicts);

    }
    fclose(statsFile);



}