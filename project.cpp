#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <cmath>
#include <algorithm>
using namespace std;
using ll = long long int;
using bit = int;
//////////////////////////|/////////////////////////////////////////////////////////|//////////////////////////
//////////////////////////|                                                         |//////////////////////////
//////////////////////////|            Normal version with optimal off              |//////////////////////////
//////////////////////////|                                                         |//////////////////////////
//////////////////////////|/////////////////////////////////////////////////////////|//////////////////////////
string config;
string mode = "!opt";
string f_mode = "!" + mode;
bool dbg = false;
ll Address_bits;
ll Block_size;
ll Cache_sets;
ll Associativity;
vector<vector<double>> Cor;
vector<vector<double>> Qua;
vector<int> indexing_bit;
vector<bool> indexing;
vector<string> tmp_rev_ref_block_addr;

ll str_to_int(string str){          //config reading
    //if(str.length()==0) return 0;
    //return ll(stoi(str));
    ll sum = 0,i = 0;
    while (isdigit(str[i])) {
        sum = sum * 10 + str[i] - '0';
        i++;
    }
    return sum;
}

ll bin_to_int(string str){          //binary convert to int
    //if(str.length()==0) return 0;
    //return ll(stoi(str, 0, 2));
    ll sum = 0, i = 0;
    while (isdigit(str[i])) {
        sum = sum * 2 + str[i] - '0';
        i++;
    }
    return sum;
}

ll int_to_log2(ll byte){            //int convert to binary length
    return ll(log2(double(byte)));
}

struct Block{                       //block info: byte_addr, idx, tag, opt_idx, opt_tag
    string addr;                    //byte_addr
    string tag; 
    string idx;
    string opt_tag;
    string opt_idx;
};

struct reference{                   //reference info: id, block, hit
    ll id;
    Block* block;
    bool hit;
};

struct Way{                         //way info: block, NRU, empty
    Block* block = NULL;
    bool NRU = true;
    bool empty = true;
};

struct Set{                         //set info: way
    vector<Way*> way;
};

struct Cache{                       //cache info: set
    vector<Set*> set;
};

bool hit_or_miss(string tag, Set* set, string mode){
    if(tag.length()==0) return false;
    //cout << "hit/miss " << set->way.size() << " way/set\n"; 
    //vector<ll> tmp;
    vector<string> tmp;
    for(auto i: set->way){
        if(!i->empty){
            if(mode == "opt"){
                //tmp.push_back(bin_to_int(i->block->opt_tag));
                tmp.push_back((i->block->opt_tag));
            }
            else{
                //tmp.push_back(bin_to_int(i->block->tag));
                tmp.push_back((i->block->tag));
            }
            //cout << i->block->tag << ", ";
        }         
    } 
    //cout << "\n";
    //cout << tmp.size() << " here\n";
    sort(tmp.begin(), tmp.end()); 
    //cout << "sorted\n";
    //bool rslt = binary_search(tmp.begin(), tmp.end(), ll(bin_to_int(tag)));
    //cout << ll(bin_to_int(tag)) << "\n";
    //cout << tag << "\n";
    //bool rslt = search_string(tmp, tag);
    bool rslt = binary_search(tmp.begin(), tmp.end(), tag);
    //if (rslt) 
    return rslt;
}

reference reference_initiate(int Id, Block* block){
    reference tmp;
    tmp.id = Id;
    tmp.block = block;
    tmp.hit = false;
    return tmp;
}

Block* Block_initiate(string Byte_addr, string Tag, string Idx){
    Block* tmp = new Block;
    tmp->addr = Byte_addr;
    tmp->tag = Tag;
    tmp->idx = Idx;
    return tmp;
}

void Set_initiate(Set* A, ll Associativity){
    for (int i=0; i<Associativity; i++){
        Way* tmp = new Way;
        A->way.push_back(tmp);
    } 
    //cout << "create " << A->way.size() << " way per set\n";
}

Cache* Cache_initiate(ll Cache_sets, ll Associativity){
    Cache* tmp = new Cache;
    for (int i=0; i<Cache_sets; i++){
        Set* A = new Set;
        Set_initiate(A, Associativity);
        tmp->set.push_back(A);
    }
    //cout << "create " << tmp->set.size() << " set per cache\n";
    //cout << "Cache Initiate Success!\n";
    return tmp;
}

int find_empty_way_NRU(Set* A, ll Associativity){
    for(int i=0; i<Associativity; i++){    //search for first empty_true
        if(A->way[i]->empty){
            if (dbg) cout << "find_empty_way[" << i << "]\n";
            A->way[i]->empty = false;
            A->way[i]->NRU = false;
            return i;
        }
    }
    for(int i=0; i<Associativity; i++){    //search for first NRU_true
        if(A->way[i]->NRU){
            if (dbg) cout << "find_NRU_way[" << i << "]\n";
            A->way[i]->empty = false;
            A->way[i]->NRU = false;
            return i;
        }
    }
    for(int i=0; i<Associativity; i++){    //all empty_false and NRU_false, reset all NRU_true
        A->way[i]->NRU = true;
    }
    if (dbg) cout << "no empty way, rest all way NRU\n";
    int i;
    for(i=0; i<Associativity; i++){    //search for first NRU_true
        if(A->way[i]->NRU){
            if (dbg) cout << "find_victim_way[" << i << "] " << A->way[i]->NRU <<"\n";
            A->way[i]->empty = false;
            A->way[i]->NRU = false;
            break;        
        }
    }
    return i;
}

bool same_bit(char A, char B){
    return A==B;
}

vector<bool> indexing_map(vector<int> indexing_bit, ll Address_bits, ll Cache_sets){
    int num_of_indexing_bit = log2(Cache_sets);
    vector<bool> tmp(Address_bits, false);
    if (dbg) cout << "indexing_map\n";
    for(int i=0; i<num_of_indexing_bit ;i++){
        tmp[indexing_bit[i]]=true;
        if (dbg) cout << indexing_bit[i] << " ";
    }
    if (dbg) cout << "\n";
    for(auto i: tmp){
        if (dbg) cout << i << " ";
    }
    if (dbg) cout << "\n\n";
    return tmp;
}

string extract_idx(string reversed_block_addr, vector<bool> indexing, ll block_addr_bit){
    int off_set = log2(Block_size);
    //reverse(block_addr.begin(), block_addr.end());
    string tmp;
    if (dbg) cout << "extract_idx ";
    /*for(int i=off_set; i<off_set+block_addr_bit; i++){
        if(indexing[i]){
            if (dbg) cout << i << " ";
            tmp = tmp + block_addr[i-off_set];
        }
    }*/
    for(int i=off_set+block_addr_bit-1; i>=off_set; i--){
        if(indexing[i]){
            if (dbg) cout << i << " ";                   //indexing : true, bits for indexing
            tmp = tmp + reversed_block_addr[i-off_set];  //byte_address 0->off_set+block_addr_bit-1        //[0][1][2][3][4][5][6][7]
            //cout << reversed_block_addr << "->";       //block_address offset->off_set+block_addr_bit-1  //[ ][ ][2][3][4][5][6][7]
        }
    }
    if (dbg) cout << ", ";
    //reverse(tmp.begin(), tmp.end());                   //for loop read in reverse to get tmp now tmp is same order as input
    return tmp;
}

string extract_tag(string reversed_block_addr, vector<bool> indexing, ll block_addr_bit){
    int off_set = log2(Block_size);
    //reverse(block_addr.begin(), block_addr.end());
    string tmp;
    if (dbg) cout << "extract_tag ";
    /*for(int i=off_set; i<off_set+block_addr_bit; i++){
        if(!indexing[i]){
            if (dbg) cout << i << " ";
            tmp = tmp + block_addr[i-off_set];
        }
    }*/
    for(int i=off_set+block_addr_bit-1; i>=off_set; i--){
        if(!indexing[i]){
            if (dbg) cout << i << " ";                   //indexing : true, bits for indexing
            tmp = tmp + reversed_block_addr[i-off_set];  //byte_address 0->off_set+block_addr_bit-1        //[0][1][2][3][4][5][6][7]
            //cout << reversed_block_addr << "->";       //block_address offset->off_set+block_addr_bit-1  //[ ][ ][2][3][4][5][6][7]
        }
    }
    if (dbg) cout << "\n";
    //reverse(tmp.begin(), tmp.end());                   //for loop read in reverse to get tmp now tmp is same order as input
    return tmp;
}

void block_opt_info(vector<reference> reference_Block, vector<bool> indexing, ll block_addr_bit){
    if (dbg) cout << "\nextrax_idx_tag\n";
    int ref_len=reference_Block.size();
    vector<string> tmp_ref_block_addr;
    for(auto i :reference_Block){
        string tmp_block_addr = i.block->tag+i.block->idx;
        reverse(tmp_block_addr.begin(), tmp_block_addr.end());
        tmp_ref_block_addr.push_back(tmp_block_addr);
    }
    for(int i=0; i<ref_len; i++){
        string reversed_block_addr = tmp_ref_block_addr[i];
        //string reversed_block_addr = i.block->tag+i.block->idx;
        //reverse(reversed_block_addr.begin(), reversed_block_addr.end());
        reference_Block[i].block->opt_idx = extract_idx(reversed_block_addr, indexing, block_addr_bit);
        reference_Block[i].block->opt_tag = extract_tag(reversed_block_addr, indexing, block_addr_bit);
    }
    if (dbg) cout << "\nblock_opt_info\n";
    for(auto i:reference_Block){
        if (dbg) cout << "____tag: " << i.block->tag << " + ____idx: " << i.block->idx << ", \n";
        if (dbg) cout << "opt_tag: " << i.block->opt_tag << " + opt_idx: " << i.block->opt_idx << "\n";
    }
    if (dbg) cout << "\n";
}

vector<vector<double>> Correlation(vector<reference> reference_Block, ll block_addr_bit){
    int ref_Block_len = reference_Block.size();
    int last_addr_idx = Address_bits - 1;// - log2(Block_size);
    int off_set = log2(Block_size); 
    if (dbg) cout << "last_addr_idx: " << last_addr_idx << "\n";
    vector<vector<double>> cor(Address_bits, vector<double> (Address_bits, 0));
    //vector<string> tmp_rev_ref_block_addr;
    for(auto i :reference_Block){
        string tmp_block_addr = i.block->tag+i.block->idx;
        reverse(tmp_block_addr.begin(), tmp_block_addr.end());
        tmp_rev_ref_block_addr.push_back(tmp_block_addr);
    }
    for(int i=0; i<block_addr_bit-1; i++){
        for(int j=i+1; j<block_addr_bit; j++){
            double E=0, D=0, C=0;
            for(int k=0; k<ref_Block_len; k++){
                string block_addr = tmp_rev_ref_block_addr[k];
                //string block_addr = reference_Block[k].block->tag + reference_Block[k].block->idx;
                //cout << block_addr << "->";
                //reverse(block_addr.begin(), block_addr.end());
                //cout << block_addr << "\n";
                if(same_bit(block_addr[i], block_addr[j])){
                    E++;
                }
                else{
                    D++;
                }

            }
            //cout <<"hey";
            cor[off_set+i][off_set+j] = min(D, E) / max(D, E);
            cor[off_set+j][off_set+i] = cor[off_set+i][off_set+j];
        }
    }
    return cor;
}

vector<vector<double>> Quality(vector<reference> reference_Block, ll block_addr_bit, ll Index_Set_bit, vector<vector<double>> Cor){
    int ref_Block_len = reference_Block.size();
    int last_addr_idx = Address_bits - 1;
    int off_set = log2(Block_size);
    if (dbg) cout << "last_addr_idx: " << last_addr_idx << "\n\n";
    vector<vector<double>> qua(block_addr_bit, vector<double> (Address_bits, 0));
    vector<bool> select(Address_bits, false);
    double Max_Q=0;
    int Max_Q_idx=0;
    /*vector<string> tmp_ref_block_addr;
    for(auto i :reference_Block){
        string tmp_block_addr = i.block->tag+i.block->idx;
        reverse(tmp_block_addr.begin(), tmp_block_addr.end());
        tmp_ref_block_addr.push_back(tmp_block_addr);
    }*/
    for(int i=0; i<block_addr_bit; i++){     
        if(i==0){            
            for(int j=0; j<block_addr_bit ; j++){
                double Z=0, O=0;
                for(int k=0; k<ref_Block_len; k++){
                    string block_addr = tmp_rev_ref_block_addr[k];
                    //string block_addr = reference_Block[k].block->tag + reference_Block[k].block->idx;                    
                    //reverse(block_addr.begin(), block_addr.end());
                    if(block_addr[j]=='1'){
                        O++;
                    }
                    else{
                        Z++;
                    }
                }
                qua[i][off_set+j] = min(Z, O) / max(Z, O);
                if(qua[i][off_set+j]>=Max_Q){
                    Max_Q = qua[i][off_set+j];
                    Max_Q_idx = off_set+j;
                }
            }
            if (dbg) cout << "Max_Q = " << qua[i][Max_Q_idx] << ", idx = " << Max_Q_idx << "\n";
            select[Max_Q_idx] = true;
            indexing_bit.push_back(Max_Q_idx);
        }    
        else{
            Max_Q = 0;
            for(int j=0; j<block_addr_bit ; j++){
                qua[i][off_set+j] = qua[i-1][off_set+j]*Cor[Max_Q_idx][off_set+j];
            }
            for(int j=0; j<block_addr_bit ; j++){
                if(qua[i][off_set+j]>=Max_Q && !select[off_set+j]){
                    Max_Q = qua[i][off_set+j];
                    Max_Q_idx = off_set+j;
                }
            }
            if (dbg) cout << "Max_Q = " << qua[i][Max_Q_idx] << ", idx = " << Max_Q_idx << "\n";
            select[Max_Q_idx] = true;
            indexing_bit.push_back(Max_Q_idx);
        }
    }
    return qua;
}

void debug_qua(vector<vector<double> > &A){
    cout << "\n-----debug quality-----\n";
    for(auto i: A){
        for(auto j: i){
            cout << setw(5) << setprecision(2) << j;
        }
        cout << "\n";
    }
    cout << "\n";
}

void debug_cor(vector<vector<double>> &A){
    cout << "\n-----debug correlation-----\n";
    for(auto i: A){
        for(auto j: i){
            cout << setw(5) << setprecision(2) << j;
        }
        cout << "\n";
    }
    cout << "\n";
}

void debug_cache(Cache* C, ll len, string mode){
    cout << "\n-----debug cache-----\n";
    char a = '-';
    string empty(len, a);
    for(auto i : C->set){
        for(auto j: i->way){
            if(!j->empty){
                if(mode == "opt"){
                    cout << j->block->opt_tag <<", ";
                }
                else{
                    cout << j->block->tag <<", ";
                }
            } 
            else{
                cout << empty <<", ";
            } 
        }
        cout << "\n";
    }
}

void debug_reference(vector<reference> &R){
    cout << "\n-----debug reference-----\n";
    for(auto i : R){
        cout << "ref_ID: " << setw(3) << i.id << ", Tag " << i.block->tag << ", idx " << bin_to_int(i.block->idx);
        cout << ", opt_Tag " << i.block->opt_tag << ", opt_idx " << bin_to_int(i.block->opt_idx);
        if(i.hit) cout << " hit\n";
        else cout << " miss\n";
    }
}

void debug_NRU(Set* S){
    int j=0;
    for(auto i : S->way){
        cout << "[" << j++ << "] :" << i->NRU <<", ";
        if(i->empty) cout << "empty, ";
        else cout << "used, ";
    }
    cout<<"\n";
}

int main(int argc, char *argv[]){  
	ifstream fin;
	ofstream fout; 
       
//====================================== Cache_Config ====================================== 
    
    fin.open(argv[1], ios::in);       //read cache.org
    for(int i=0; i<4; i++){
        getline(fin, config,'\n');
        if(config[1]=='d'){           //Address_bits
            size_t found = config.find(" ");
            Address_bits = str_to_int(config.substr(found+1));
        }
        else if(config[1]=='l'){      //Block_size
            size_t found = config.find(" ");
            Block_size = str_to_int(config.substr(found+1));
        }
        else if(config[1]=='a'){      //Cache_sets
            size_t found = config.find(" ");
            Cache_sets = str_to_int(config.substr(found+1));
        }
        else if(config[1]=='s'){      //Associativity
            size_t found = config.find(" ");
            Associativity = str_to_int(config.substr(found+1));
        }
    }
    fin.close();
    cout << "\nAddress_bits "<< Address_bits << ", Block_size(byte/block) " << Block_size  << ", Cache_sets(set/cache) " << Cache_sets << ", Associativity(block/set) "<< Associativity <<"\n\n";
   
//====================================== Cache_Bench ======================================   
    
    fin.open(argv[2], ios::in);       //read reference.lst
    vector<reference> reference_Block;
    string ref_addr;
    ll Offset_bit = ll(log2(Block_size));
	ll Index_Set_bit = ll(log2(Cache_sets));
    ll Tag_bit = Address_bits - Index_Set_bit - Offset_bit;
    ll block_addr_bit = Address_bits - Offset_bit;
    ll num_ref = 0;
    cout << "\nTag_bit "<< Tag_bit << ", offset_bit(byte/block) " << Offset_bit << ", Index_Set_bit(set/cache) " << Index_Set_bit << "\n\n";
    string testcase;
    fin>>testcase;
    fin>>testcase;
    while(getline(fin, ref_addr,'\n')){
		if (fin.eof()){
			break;
		}else if (ref_addr.size()==0 || ref_addr[1]=='b' || ref_addr[1]=='e'){
			continue;
		}else{   
            Block* tmp = Block_initiate(ref_addr, ref_addr.substr(0, Tag_bit), ref_addr.substr(Tag_bit, Index_Set_bit));
            reference rf = reference_initiate(num_ref, tmp);
            reference_Block.push_back(rf);
            num_ref++;
        }
	}
    fin.close();
    for (auto i : reference_Block){
        if (dbg) cout << "ref_ID: " << setw(3) << i.id <<", block_add "<< i.block->tag+i.block->idx <<", Tag " << i.block->tag << ", idx " << bin_to_int(i.block->idx) << " (" << i.block->idx << ")\n";
    }  
    cout << "\n";

//====================================== Correlation_&_Quality ======================================    

    if(mode == "opt"){
        Cor = Correlation(reference_Block, block_addr_bit);
        if (dbg) debug_cor(Cor);
        Qua = Quality(reference_Block, block_addr_bit, Index_Set_bit, Cor);
        if (dbg) debug_qua(Qua);
        indexing = indexing_map(indexing_bit, Address_bits, Cache_sets);
        block_opt_info(reference_Block, indexing, block_addr_bit); 
    }

//====================================== Cache_NRU_mode ======================================

    Cache* cache = Cache_initiate(Cache_sets, Associativity);
    for(auto &i : reference_Block){
        if (dbg) cout << "\n============ ref_ID : " << i.id << " ============";
        if(Index_Set_bit == 0){       //fully_associate
            //cout << "\nfully assocaite\n";
            //cout << "ref_ID : " << i.id << " tag " << i.block->tag << " idx(set) 0\n";
            if (dbg) debug_NRU(cache->set[0]);
            if (dbg) debug_cache(cache, Tag_bit, f_mode);
            if (hit_or_miss(i.block->tag, cache->set[0], f_mode) == true){           //hit
                if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->tag << " hit ! " << " idx(set) 0\n";
                i.hit = true;
            }  
            else{                                                            //miss
                if (dbg) cout << "\nref_ID : " << i.id << " "<< i.block->tag << " miss !"  << " idx(set) 0\n";
                i.hit = false;  
                int victim = find_empty_way_NRU(cache->set[0], Associativity);
                cache->set[0]->way[victim]->block = i.block;
                cache->set[0]->way[victim]->empty = false;
                cache->set[0]->way[victim]->NRU = false;
            }  
            //debug_NRU(cache->set[0]);
            if (dbg) debug_cache(cache, Tag_bit, f_mode);           
        }
        else if(Associativity == 1){  //direct_map
            //cout << "\ndirect map\n";                                        
            int index ;
            if(mode == "opt"){
                //cout << "ref_ID : " << i.id << " opt_tag " << i.block->opt_tag << " opt_idx(set) " << stoi(i.block->opt_idx, 0, 2) << "\n";
                index = bin_to_int(i.block->opt_idx);
                if (dbg) debug_NRU(cache->set[index]);
                if (dbg) debug_cache(cache, Tag_bit, mode);
                if (hit_or_miss(i.block->opt_tag, cache->set[index], mode) == true){       //hit
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->opt_tag << " hit !" << " opt_idx(set) " << index << "\n";
                    i.hit = true;
                }  
                else{                                                            //miss
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->opt_tag << " miss !" << " opt_idx(set) " << index << "\n";  
                    i.hit = false;  
                    cache->set[index]->way[0]->block = i.block;
                    cache->set[index]->way[0]->empty = false;
                    cache->set[index]->way[0]->NRU = false;
                    //cout << cache->set[index].way[0].block. << "\n";
                }
            }
            else{
                //cout << "ref_ID : " << i.id << " tag " << i.block->tag << " idx(set) " << stoi(i.block->idx, 0, 2) << "\n";
                index = bin_to_int(i.block->idx);
                if (dbg) debug_NRU(cache->set[index]);
                if (dbg) debug_cache(cache, Tag_bit, mode);
                if (hit_or_miss(i.block->tag, cache->set[index], mode) == true){       //hit
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->tag << " hit !" << " idx(set) " << index << "\n";
                    i.hit = true;
                }  
                else{                                                            //miss
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->tag << " miss !" << " idx(set) " << index << "\n";  
                    i.hit = false;  
                    cache->set[index]->way[0]->block = i.block;
                    cache->set[index]->way[0]->empty = false;
                    cache->set[index]->way[0]->NRU = false;
                    //cout << cache->set[index].way[0].block. << "\n";
                } 
            }
            //debug_NRU(cache->set[index]); 
            if (dbg) debug_cache(cache, Tag_bit, mode); 
        }
        else{                         //n_way_associate
            //cout << "\n" << Associativity << " way\n";
            int index;
            if (mode == "opt"){
                //cout << "ref_ID : " << i.id << " opt_tag " << i.block->opt_tag << " opt_idx(set) " << stoi(i.block->opt_idx, 0, 2) << "\n";
                index = bin_to_int(i.block->opt_idx);
                if (dbg) debug_NRU(cache->set[index]);
                if (dbg) debug_cache(cache, Tag_bit, mode);
                if (hit_or_miss(i.block->opt_tag, cache->set[index], mode) == true){       //hit
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->opt_tag << " hit !" << " opt_idx(set) " << index << "\n";
                    i.hit = true;               
                }  
                else{                                                            //miss
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->opt_tag << " miss !" << " opt_idx(set) " << index << "\n";  
                    i.hit = false;  
                    int victim = find_empty_way_NRU(cache->set[index], Associativity);
                    cache->set[index]->way[victim]->block = i.block;
                    cache->set[index]->way[victim]->empty = false;
                    cache->set[index]->way[victim]->NRU = false;
                    //cout << cache->set[index].way[0].block. << "\n";
                }
            }
            else{
                //cout << "ref_ID : " << i.id << " tag " << i.block->tag << " idx(set) " << stoi(i.block->idx, 0, 2) << "\n";
                index = bin_to_int(i.block->idx);
                if (dbg) debug_NRU(cache->set[index]);
                if (dbg) debug_cache(cache, Tag_bit, mode);
                if (hit_or_miss(i.block->tag, cache->set[index], mode) == true){       //hit
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->tag << " hit !" << " idx(set) " << index << "\n";
                    i.hit = true;               
                }  
                else{                                                            //miss
                    if (dbg) cout << "\nref_ID : " << i.id << " " << i.block->tag << " miss !" << " idx(set) " << index << "\n";  
                    i.hit = false;  
                    int victim = find_empty_way_NRU(cache->set[index], Associativity);
                    cache->set[index]->way[victim]->block = i.block;
                    cache->set[index]->way[victim]->empty = false;
                    cache->set[index]->way[victim]->NRU = false;
                    //cout << cache->set[index].way[0].block. << "\n";
                }
            }
            if (dbg) debug_NRU(cache->set[index]);  
            if (dbg) debug_cache(cache, Tag_bit, mode); 
        }
    }
    if (dbg) debug_cache(cache, Tag_bit, mode);   
    if (dbg) debug_reference(reference_Block);

//====================================== output_result ====================================== 

    fout.open(argv[3], ios::out);     //write_index.rpt 
    fout << "Address bits: " << Address_bits << "\n";
    fout << "Block size: " << Block_size << "\n";
	fout << "Cache sets: " << Cache_sets << "\n";
	fout << "Associativity: " << Associativity << "\n";
	fout << "\n";
    fout << "Offset bit count: " << Offset_bit << "\n";
    fout << "Indexing bit count: " << Index_Set_bit << "\n";
	fout << "Indexing bits: ";
    for(int i=0; i<Index_Set_bit ;i++){
        if(mode == "opt"){
            fout << indexing_bit[i] << " " ;
        }
        else{
            fout << (Address_bits - 1) - Tag_bit - i << " " ;
        }       
    }
    fout << "\n\n";
    fout << ".benchmark " << testcase <<"\n";
    int miss = 0;
    for(auto i : reference_Block){
        fout << i.block->addr << " ";
        if(i.hit){
            fout << "hit\n";
        }
        else{
            fout << "miss\n";
            miss++;
        }
    }
    fout << ".end\n";
	fout << "\n";
    fout << "Total cache miss count: " << miss << "\n";
    fout.close();    

    return 0;
}

