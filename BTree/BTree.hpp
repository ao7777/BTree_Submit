#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <fstream>
#include <map>
#include<cstring>
namespace sjtu
{
template <class Key, class Value, class Compare = std::less<Key>>
class BTree
{
private:
    //这里是一些数据成员
    //运行时除了常用的数据的常量和info，buffer，name及文件流对象f之外，其余变量全部位于文件中
    const static size_t M;
    static short L;
    const size_t MAX_FNAME=50;
    const size_t NODESIZE=3000;
    const size_t bufferSize = 4096;
    static int ID=1;
    class iterator;
    struct dataInfo{
        //head: start of data link
        //tail: end of data link
        //root: root of the tree
        size_t head, tail, root, size, eof;
        char name[MAX_FNAME+1];
        dataInfo(){
            name="";
            head = tail = root = size = eof = 0;
        }
    }info;
    struct node{
        char nodeType;
        short sz;
        size_t pos, next, prev,parent;
        Key keys[M-1];
        char data[NODESIZE];
        node()：sz(1){
            nodeType = '0';
            parent = pos = next = prev = 0;
        }
    };
    //BufferManager
    char *buffer;
    node pres;
    std::fstream f;
    inline void setName(char *n){
        if(strlen(n)>MAX_FNAME)
            throw "invaild file name";
        strcpy(info.name, n);
    }
    inline void write(size_t pos=0){
        //write data in buffer into a designated node
		if (f.fail())
			f.close();
        if(!fStat.open){
            f.open(info.name,std::ios::binary);
            if(f.good()){
                if(!pos)//write at the end of file by default
                {
                    f.seekg(0, std::ios::end);
                    pos = f.tellg();
                }
                f.seekp(pos);
                memcpy(buffer, p, sizeof(node));
                f.write(buffer, sizeof(char) * M)
                f.flush();
                return true;
                    }
            else {
                    f.close();
					throw "Fail to write the file. ";
                }
        }
    }
    inline void open(){
        if(!f.is_open())
            f.open(info.name, std::ios::binary);
        if(!f.good())
            throw "Fail to open the file";
    }
    inline void initialize(){//将基本信息写入文件
        f.seekp(0);
        pos = 0;
        memcpy(&(buffer[pos]), info.name, sizeof(char) * MAX_FNAME);
        pos += sizeof(char) * MAX_FNAME;
        memcpy(&(buffer[pos]), &info.size, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.head, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.tail, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.root, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.eof, sizeof(size_t));
        f.write(buffer, bufferSize);
        f.flush();
    }
    inline void close(){
        if(f.is_open())
            f.close(); 
    }
    inline void read(size_t pos){
        //read data from a specified node into buffer
        f.seekg(pos,std::ios::beg);
        f.read(buffer,sizeof(node));
        int i=0;
        //copy data into node pres
        memcpy(&pres,(buffer+i),sizeof(char));
        i+=sizeof(char);
        memcpy(&pres,(buffer+i),sizeof(short));
        i+=sizeof(short);
        for(int j=0;j<4;j++){
        memcpy(&pres,(buffer+i),sizeof(size_t));
        i+=sizeof(size_t);
        }
        memcpy(&pres,(buffer+i),sizeof(Key)*(M-1));
        i+=sizeof(Key)*(M-1);
        memcpy(&pres, (buffer + i), sizeof(char) * M);
        i+=sizeof(size_t)*L;
    }
    inline void copy(const BTree &other){
        //封装的copy函数，实现文件操作
        size_t n;
        n=other.info.eof%other.M;
        for(size_t i=0;i<=n+1;i++){
            f.seekp(i*M,std::ios::beg);
            other.f.seekg(i*M,std::ios::beg);
            other.f.read(buffer,M);
            f.write(buffer,sizeof(char)*M);//每次读入4K字节的数据并写入
        }
    }
    bool insertIdx(pair<Key,size_t> _p,size_t block){
        read(block);
        size_t par=pres.parent;
        if(pres.sz!=M-1){//this node is availble
            char buffer0[bufferSize];
            Key p[M];
            size_t i = 0;
            while((pres.keys[i]<_p.first&&i<M-1)
                i++;
            memcpy(buffer0,pres.data,sizeof(size_t)*pres.sz);
            *(((size_t *)pres.data)+i)=_p.second;
            memcpy(p,pres.keys+i,sizeof(Key)*(M-i));//copy key
            pres.keys[i]=k;
            memcpy(pres.keys+i+1,p,sizeof(Key)*(M-i-1));
            memcpy(buffer+sizeof(size_t)*pres.sz,buffer0,sizeof(size_t)*(pres.sz-i));
            memcpy(pres.data,buffer,sizeof(size_t)*pres.sz+1);
            pres.sz++;
            memcpy(buffer,pres,sizeof(node));
            write(pres.pos); 
        }
        else{//this node is full
            pair<Key,size_t> __p=restrIdx(_p,block);
            if(block==info.root){
                node tmp;
                tmp.nodeType='0';
                tmp.sz=1;
                f.seekg(0,std::ios::end);
                tmp.pos=f.tellg();
                tmp.prev=tmp.next=tmp.parent=0;
                tmp.keys[0]=__p.first;
                *((size_t *)tmp.data)=__p.second;
                memcpy(buffer,tmp,sizeof(node));
                write();
            }
            else{
                insertIdx(__p,par);
            }
        }
    }
    pair<Key,size_t> restrIdx(pair<Key,size_t> _p,size_t pos){
        //insert k-p pair into idxnode and split it 
        //into halfs then return the k-p pair in the middle
        read(pos);
        int i=0;
        while(pres.keys[i]<_p.first&&i<M-1)i++;
        size_t n,p,s=M,tmp2[M+1];
        Key tmp[M];
        char buffer0[bufferSize];
        int j=sizeof(char);
        *(short *)(buffer+i)=s/2;        
        j+=sizeof(short)+sizeof(size_t);
        f.seekg(0,std::ios::end);
        p=f.tellg();
        *(size_t *)(buffer+i)=p;
        j+=sizeof(size_t)*3;
        memcpy(buffer0,buffer,j);
        memcpy(tmp,buffer+j,sizeof(Key)*i);
        j+=sizeof(Key)*i;
        tmp[i]=_p.first;
        memcpy(tmp,buffer+j,sizeof(Key)*(M-i-1));
        memcpy(buffer+j-sizeof(Key)*(M-1),tmp,sizeof(Key)*((M-1)/2));
        j+=sizeof(Key)*(M-i);
        memcpy(tmp2,buffer+j,sizeof(size_t)*(i+1));
        j+=sizeof(size_t)*i;
        tmp2[i+1]=_p.second;
        memcpy(tmp,buffer+j,sizeof(size_t)*(M-i-1));
        memcpy(buffer+j-sizeof(size_t)*M,tmp2,sizeof(size_t)*M/2);
        write(pres.pos);
        i=sizeof(char)+sizeof(short)+sizeof(size_t);
        if(pres.next){
            read(pres.next);
            *(size_t *)(buffer0+i)=pres.pos;
        }
            i+=sizeof(size_t);
            *(size_t *)(buffer0+i)=pos;
        if(pres.next){ 
            *(size_t *)(buffer+i)=p;    
            write(pres.pos);
        }
        i+=2*sizeof(size_t);
        memcpy(buffer0+i,tmp+(M-1)/2+1,sizeof(Key)*(s-1)/2));
        i+=sizeof(Key)*(M-1);
        memcpy(buffer0+i,tmp2+M/2+1,sizeof(size_t)*s/2);
        i-=sizeof(size_t)*4+sizeof(Key)*(M-1);
        *(size_t *)(buffer0+i)=p;            
        i-=sizeof(short);
        *(size_t *)(buffer0+i)=s/2;
        //buffer at now :data of the next node(if pres.next)    
        memcpy(buffer,buffer0,bufferSize);
        write();
        pair<Key,size_t> __p(tmp1[(M-1)/2-1],tmp2[M/2-1]);
        return __p;
    }
    Key split(size_t pos,bool flag){
        //split a node at the half of it
        //return the key of the first element
        //in the second split node
        size_t n,p,s;
        if(flag)s=L;
        else s=M;
        Key k;
        read(pos);
        char buffer0[bufferSize];
        memcpy(buffer0,buffer,bufferSize);
        k=pres.keys[s/2];
        int i=sizeof(char);
        *(short *)(buffer+i)=s/2;
        i+=sizeof(short)+sizeof(size_t);
        f.seekg(0,std::ios::end);
        p=f.tellg();
        *(size_t *)(buffer+i)=p;//A->B
        write(pres.pos);
        if(pres.next){
            read(pres.next);
            *(size_t *)(buffer0+i)=pres.pos;
        }
            i+=sizeof(size_t);
            *(size_t *)(buffer0+i)=pos;
        if(pres.next){ 
            *(size_t *)(buffer+i)=p;    
            write(pres.pos);
        }
            i+=2*sizeof(size_t);
            memcpy(buffer0+i,buffer0+i+sizeof(Key)*s/2,sizeof(Key)*(s/2-1));
            i+=sizeof(Key)*(M-1);
            if(flag)memcpy(buffer0+i,buffer0+i+sizeof(Value)*s/2,sizeof(Value)*s/2);
            else memcpy(buffer0+i,buffer0+i+sizeof(size_t)*s/2,sizeof(size_t)*s/2);
            i-=sizeof(size_t)*4+sizeof(Key)*(M-1);
            *(size_t *)(buffer0+i)=p;            
            i-=sizeof(short);
            *(size_t *)(buffer0+i)=s/2;
            
        //buffer at now :data of the next node(if pres.next)    
        memcpy(buffer,buffer0,bufferSize);
        write();
        //this function doesn't change parent node
        return k;
        //after this function pres is the node next to the
        //second node
    }
    void merge(size_t pos){
        //merge a node and the next node
        read(pos);//读入当前结点
        int n,sz,i=0;
        Key k;
        sz=pres.sz;
        char buffer0[bufferSize];
        memcpy(buffer0,buffer,bufferSize);
        read(pres.next);
        i+=sizeof(char)+sizeof(short)+sizeof(size_t)*3+
        sizeof(Key)*(M-1);
        memcpy(buffer0+i+sz,buffer+i,pres.sz);
        i-=sizeof(Key)*(M-1)+sizeof(size_t)*2;
        *(size_t *)(buffer0+i)=pres.next;    
        write(pres.prev);
        if(pres.next){
        read(pres.next);
        pres.prev=pos;
        memcpy(buffer,&pres,bufferSize);
        write(pres.pos);
        }
        
        //this function doesn't change parent node
    }
    iterator _find(const Key &key, bool &flag){
        short l,r,mid;
        read(info.root);
        size_t pos;
        while(pres.nodeType){
        l=0;
        r=pres.sz-1;
        while(l<r){
            mid=(l+r)/2;
            if(pres.keys[mid]>=key)
                if(pres.keys[mid-1]<key)break;
                else r=mid-1;
            else if(pres.keys[mid+1]>key){
                mid++;
                break;
            }
            else l=mid+1;
        }
        memcpy(&pos,&((size_t *)pres.data+mid),sizeof(size_t));
        read(pos);
        p=mid;
        }
        l=0;
        r=pres.sz-1;
        while(l<r){
            mid=(l+r)/2;
            if(pres.keys[mid]>key)
                r=mid-1;
            else l=mid+1;
        }
        if(l==r)flag=1;
        else flag=0;
        iterator it(pres.pos,pres.sz,r,this);
        return it;
    }
    void build(){
        //initialize a Btree;
        info.size = 0;
        info.head = 0;
        info.eof = sizeof(basicInfo);
        node root;
        info.root = root.pos=info.eof;
        info.eof += sizeof(node);
        root.nodeType = '\0';
        open();
        initialize();
        
    }
public:
    typedef pair<const Key, Value> value_type;
    class const_iterator;
    class iterator
    {
    private:
        // Your private members go here
        size_t blockPos;
        size_t blockSize;
        short dataPos;
        BTree  *tree;
    public:
        bool modify(const Value &value)
        {
        }
        iterator()
        {
            blockPos=dataPos=0;
            // TODO Default Constructor
        }
        iterator(size_t b,size_t s,short d, BTree *t):blockPos(b),blockSize(s),dataPos(d),tree(t){}
        iterator(const iterator &other)
        {
            blockPos=other.blockPos；
            dataPos=other.dataPos;
            blockSize=other.blockSize;
            // TODO Copy Constructor
        }
        // Return a new iterator which points to the n-next elements
        iterator operator++(int)
        {  
            if(blockPos+dataPos==tree->info.eof)
                throw index_out_of_bound;
            iterator tmp=*this;
            tree->read(blockPos);
            if(dataPos==pres.sz-1){
                tree->read(pres.next);//read next node
                blockPos =;
                dataPos = 0;
                blockSize=pres.sz;
            }
            else
                dataPos++;
            tree->close;
            return &tmp;
            // Todo iterator++
        }
        iterator &operator++()
        {
            *this++;
            return *this;
        }
        iterator operator--(int)
        {
            if(blockPos+dataPos==tree->info.root)
                throw index_out_of_bound;
            iterator tmp=*this;
            read(blockPos);
            if(dataPos==0){
                read(pres.prev);//read next node
                blockPos = pres.pos;
                blockSize=pres.sz;
                dataPos =  pres.sz-1;
            }
            else
                dataPos--;
            return &tmp;
            // Todo iterator--
        }
        iterator &operator--()
        {
            *this--;
            return *this;
            // Todo --iterator
        }
        iterator &operator+=(int p){
             if(blockPos+dataPos+p==tree->info.eof||blockPos+dataPos+p==tree->info.root)
                throw index_out_of_bound;
            iterator tmp=*this;
            read(blockPos);
            //wait for completion
            return &tmp;
        }
        // Overloaded of operator '==' and '!='
        // Check whether the iterators are same
        bool operator==(const iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos==rhs.blockPos&&dataPos==rhs.dataPos;
            return false;
            // Todo operator ==
        }
        bool operator==(const const_iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos==rhs.blockPos&&dataPos==rhs.dataPos;
            return false;// Todo operator ==
        }
        bool operator!=(const iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos!=rhs.blockPos||dataPos!=rhs.dataPos;
            return false;// Todo operator !=
        }
        bool operator!=(const const_iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos!=rhs.blockPos||dataPos!=rhs.dataPos;
            return false;// Todo operator !=
        }
    };
    class const_iterator
    {
        // it should has similar member method as iterator.
        //  and it should be able to construct from an iterator.
    private:
        size_t blockPos;
        size_t dataPos;
        size_t blockSize;
        BTree *tree;// Your private members go here
    public:
        const_iterator()
        {
            blockSize=blockPos=dataPos=0;
        }
        const_iterator(const const_iterator &other)
        {
            blockPos=other.blockPos；
            blockSize=pres.sz;
            dataPos=other.dataPos;
        }
        const_iterator(const iterator &other)
        {
            blockPos=other.blockPos；
            dataPos=other.dataPos;
        }
        const_iterator operator++(int)
        {  
            if(blockPos+dataPos==tree->info.eof)
                throw index_out_of_bound;
            const_iterator tmp=*this;
            read(blockPos);
            if(dataPos==pres.sz-1){
                tree->read(pres.next);//read next node
                blockPos =;
                blockSize=pres.sz;
                dataPos = 0;
            }
            else
                dataPos++;
            return &tmp;
            // Todo iterator++
        }
        const_iterator &operator++()
        {
            *this++;
            return *this;
        }
        const_iterator operator--(int)
        {
            if(blockPos+dataPos==tree->info.root)
                throw index_out_of_bound;
            const_iterator tmp=*this;
            read(blockPos);
            if(dataPos==0){
                read(pres.prev);//read next node
                blockPos = pres.pos;
                blockSize=pres.sz;
                dataPos =  pres.sz-1;
            }
            else
                dataPos--;
            return &tmp;
            // Todo iterator--
        }
        const_iterator &operator--()
        {
            *this--;
            return *this;
            // Todo --iterator
        }
        // Overloaded of operator '==' and '!='
        // Check whether the iterators are same
        bool operator==(const iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos==rhs.blockPos&&dataPos==rhs.dataPos;
            return false;// Todo operator ==
        }
        bool operator==(const const_iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos==rhs.blockPos&&dataPos==rhs.dataPos;
            return false;
            // Todo operator ==
        }
        bool operator!=(const iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos==rhs.blockPos&&dataPos==rhs.dataPos;
            return false;
            // Todo operator !=
        }
        bool operator!=(const const_iterator &rhs) const
        {
            if(rhs.tree==tree)
            return blockPos==rhs.blockPos&&dataPos==rhs.dataPos;
            return false;
            // Todo operator !=
        }
    // And other methods in iterator, please fill by yourself.
    };
    // Default Constructor and Copy Constructor
    BTree()
    {
        build();
    }
    BTree(char *s){//预设文件名的构造函数
        setName(s);
        build();
    }
    BTree(const BTree &other)
    {
        setName('0' + ID);
        ID++;
        open();
        copy(other);
        
    }
    BTree &operator=(const BTree &other)
    {
        setName('0' + ID);
        open();
        copy(other);
        
        return *this;
        // Todo Assignment
    }
    ~BTree()
    {
        close();
    }
    // Insert: Insert certain Key-Value into the database
    // Return a pair, the first of the pair is the iterator point to the new
    // element, the second of the pair is Success if it is successfully inserted
    pair<iterator, OperationResult> insert(const Key &key, const Value &value)
    { 
        if(info.size==0){
            read(info.root);
            pres.sz=1;
            pres.keys[0]=key;
            *((Value *)pres.data)=value;
            info.tail=info.head=pres.pos;
            write(pres.pos);
        }//tree is empty
        else {
            bool flag;
            size_t pos;
            iterator ins=_find(key,flag);//get the right position to insert
            if(!flag){
                //avoid inserting an element with the same key
                if(ins.blockSize<L){
                    char buffer0[bufferSize];
                    memcpy(buffer0,pres.data,sizeof(Value)*pres.sz);
                    *(((Value *)pres.data)+ins.dataPos)=value;
                    memcpy(buffer+sizeof(Value)*pres.sz,buffer0,sizeof(Value)*(pres.sz-ins.dataPos));
                    memcpy(pres.data,buffer,sizeof(Value)*pres.sz+1);
                    pres.sz++;
                    memcpy(buffer,pres,sizeof(node));
                    write(pres.pos);
                    //node is availble
                }
                else{//need to split leaf
                    size_t par=pres.parent;
                    Key k = split(ins.blockPos,true);
                    pair p(key,pres.prev);
                    insertIdx(p,par);
                }
            }
            else {
                pair<iterator,OperationResult> p(ins,Fail);
                return p;
            }   
        }
        info.size++;
        return 
    }
    // Erase: Erase the Key-Value
    // Return Success if it is successfully erased
    // Return Fail if the key doesn't exist in the database
    OperationResult erase(const Key &key)
    {
        if(info.size==0)throw container_is_empty;
        iterator it=find(key);
        if(it==end())return Fail;
        else {

        }
        info.size--;
        // TODO erase function
        return Fail; // If you can't finish erase part, just remaining here.
    }
    // Return a iterator to the beginning
    iterator begin() {
        read(info.head);
        iterator it(pres.pos,pres.sz,0,this);
        return it;
    }
    const_iterator cbegin() const {
        return begin();
    }
    // Return a iterator to the end(the next element after the last)
    iterator end() {
        read(info.tail);
        iterator it(pres.pos,pres.sz,pres.sz-1,this);
        return it;
    }
    const_iterator cend() const {
        return end();
    }
    // Check whether this BTree is empty
    bool empty() const {
        return !info.size;
    }
    // Return the number of <K,V> pairs
    size_t size() const {
        return info.size;
    }
    // Clear the BTree
    void clear() {
        build();
    }
    // Return the value refer to the Key(key)
    Value at(const Key &key)
    {
        iterator it=find(key);
        if(it==end())throw runtime_error;
        return *(((Value *)pres.data)+it.dataPos);
    }
    /**
         * Returns the number of elements with key
         *   that compares equivalent to the specified argument,
         * The default method of check the equivalence is !(a < b || b > a)
         */
    size_t count(const Key &key) const {
        iterator it=find(key);
        if(it==end())return 0;
        else return 1;
    }
    /**
         * Finds an element with key equivalent to key.
         * key value of the element to search for.
         * Iterator to an element with key equivalent to key.
         *   If no such element is found, past-the-end (see end()) iterator is
         * returned.
         */
    iterator find(const Key &key) {
        bool flag;
        iterator it=_find(key,flag);
        if(flag)return it;
        else return end();
    }
};
} // namespace sjtu
