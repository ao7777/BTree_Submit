#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <fstream>
#include<iostream>
#include <map>
#include<cstring>
namespace sjtu
{
    std::fstream f;
	int ID = 1;
template <class Key, class Value, class Compare = std::less<Key>>
class BTree
{
public:
    class iterator;
private:
    //这里是一些数据成员
    //运行时除了常用的数据的常量和info，buffer，name及文件流对象f之外，其余变量全部位于文件中
    const static size_t MAX_FNAME=50;
    const static size_t bufferSize = 4096;
	constexpr static size_t NODESIZE = (bufferSize - sizeof(char) - sizeof(short) - sizeof(size_t)*4) /(sizeof(Key)+sizeof(Value))*sizeof(Value);
	constexpr static short M = NODESIZE/sizeof(Key);
	constexpr static short L = NODESIZE/sizeof(Value);
	struct dataInfo{
        //head: start of data link
        //tail: end of data link
        //root: root of the tree
        size_t head, tail, root, size, eof;
        char name[MAX_FNAME+1];
        dataInfo(){
            name[0]='\0';
            head = tail = root = size = eof = 0;
        }
    }info;
    struct node{
        char nodeType;
        short sz;
        size_t pos, next, prev,parent;
        Key keys[M];
        char data[NODESIZE];
        node():sz(1){
            nodeType = '0';
            parent = pos = next = prev = 0;
        }
    };
    //BufferManager
    char *buffer;
    node pres;
    inline void setName(char *n){
        if(strlen(n)>MAX_FNAME)
            throw invaild_file_name();
        char names[MAX_FNAME];
        strcpy(names, n);
        strcpy(info.name, "FileNo_");
        names[0]+= ID;
        strcpy(info.name+7, names);
    }
    inline void write(size_t pos=0){
        //write data in buffer into a designated node
            if(f.good()){
                if(!pos)//write at the end of file by default
                {
                    f.seekg(0, std::ios::end);
                    pos = f.tellg();
                }
                f.seekp(pos);
                f.write(buffer, bufferSize);
                f.flush();
                    }
            else {
					throw Write_File_Failure();
                }
        }
    
    inline void open(){
        f.open(info.name, std::ios::in|std::ios::out|std::ios::binary|std::ios::ate);
        if(!f.good())
        f.open(info.name, std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
        if(!f.good())
        throw Open_File_Failure();
    }
    inline void initialize(){//将基本信息写入文件
        f.seekp(0,std::ios::beg); 
        short pos = 0;
        memcpy(&(buffer[pos]), info.name, sizeof(char) * MAX_FNAME);
        pos += sizeof(char) * (MAX_FNAME+1);
        memcpy(&(buffer[pos]), &info.size, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.head, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.tail, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.root, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(&(buffer[pos]), &info.eof, sizeof(size_t));
        f.write(buffer, sizeof(info));
        f.flush();
    }
    inline void close(){
		f.seekp(0, std::ios::beg);
		short pos = 0;
		memcpy(&(buffer[pos]), info.name, sizeof(char) * MAX_FNAME);
		pos += sizeof(char) * (MAX_FNAME + 1);
		memcpy(&(buffer[pos]), &info.size, sizeof(size_t));
		pos += sizeof(size_t);
		memcpy(&(buffer[pos]), &info.head, sizeof(size_t));
		pos += sizeof(size_t);
		memcpy(&(buffer[pos]), &info.tail, sizeof(size_t));
		pos += sizeof(size_t);
		memcpy(&(buffer[pos]), &info.root, sizeof(size_t));
		pos += sizeof(size_t);
		memcpy(&(buffer[pos]), &info.eof, sizeof(size_t));
		f.write(buffer, sizeof(info));
        if(f.is_open())
            f.close(); 
    }
    inline void read(size_t pos){
        //read data from a specified node into buffer
        f.seekg(pos,std::ios::beg);
        f.read(buffer,sizeof(node));
        //copy data into node pres
        memcpy(&pres,buffer,sizeof(node));
    }
    inline void copy(const BTree &other){
        //封装的copy函数，实现文件操作
        size_t n;
        n=other.info.eof%other.M;
        f.seekp(0);
		std::fstream of;
		of.open(other.info.name, std::ios::binary);
		if (!of.good())throw Copy_File_Failure();
        of.seekg(0,of.beg());
        for(size_t i=0;i<=n+1;i++){
            of.read(buffer,sizeof(char) * M);
            f.write(buffer,sizeof(char)*M);//每次读入4K字节的数据并写入
        }
    }
    void insertIdx(pair<Key,size_t> _p,size_t block){
		bool flag;
		read(block);
        size_t par=pres.parent;
        if(pres.sz<M-1){//this node is availble
			short i = 0;
            while(pres.keys[i]<=_p.first&&i<pres.sz)
                i++;
			for (int j = pres.sz; j >= i; j--) {
				if(j>0)pres.keys[j] = pres.keys[j - 1];
				((size_t *)pres.data)[j+1] = ((size_t *)pres.data)[j];
			}
			pres.keys[i] = _p.first;
			((size_t *)pres.data)[i+1] = _p.second;
            pres.sz++;
			memcpy(buffer,&pres,sizeof(node));
			write(block);
			
        }
        else{//this node is full
            pair<Key,size_t> __p=restrIdx(_p,block);
            if(block==info.root){//生成新的根
				node tmp;
                tmp.nodeType='0';
                tmp.sz=1;
                tmp.pos=info.eof;
                info.eof+=bufferSize;
                tmp.prev=tmp.next=tmp.parent=0;
                tmp.keys[0]=__p.first;
				*((size_t*)tmp.data) = block;
                *((size_t *)tmp.data+1)=__p.second;
                info.root = tmp.pos;
                memcpy(buffer,&tmp,sizeof(node));
                write();
				read(block);
				pres.parent = tmp.pos;
				memcpy(buffer, &pres, sizeof(node));
				write(block);
				read(__p.second);
				pres.parent = tmp.pos;
				memcpy(buffer, &pres, sizeof(node));
				write(__p.second);
            }
            else{
				read(_p.second);
				if (__p.first <= _p.first)
					pres.parent = __p.second;
				else pres.parent = block;
				memcpy(buffer, &pres, sizeof(node));
				write(_p.second);
                insertIdx(__p,par);
            }
        }
    }
    pair<Key,size_t> restrIdx(pair<Key,size_t> _p,size_t idx){
        //insert k-p pair into idxnode and split it 
        //into halfs then return the k-p pair in the middle
        size_t next,p,s=M-1,pos=0;
        read(idx);
		node tmp;
		next = pres.next;
        memcpy(&tmp,buffer,sizeof(node));
        pres.sz=s/2;
        f.seekg(0,std::ios::end);
        p=f.tellg();
        tmp.pos=pres.next=p;//A->B
		while (pres.keys[pos] <= _p.first&&pos<pres.sz)pos++;
		for (int i = 0; i < s - s / 2; i++) tmp.keys[i] = tmp.keys[i + s / 2];
		for (int i = 0; i <= s - s / 2; i++) ((size_t*)tmp.data)[i] = ((size_t*)tmp.data)[i + s / 2];
		if (pos < s / 2) {
			for (int i = s/2; i >pos; i--) {
					pres.keys[i] = pres.keys[i - 1];
					((size_t *)pres.data)[i+1] = ((size_t*)pres.data)[i];
			}
			pres.keys[pos] = _p.first;
			((size_t *)pres.data)[pos+1] = _p.second;
			pres.sz++;
		}
		memcpy(buffer, &pres, sizeof(node));
        write(pres.pos); 
		tmp.prev=idx;
        if(next){
            read(next);
            tmp.next=pres.pos;
			pres.prev = p;
			memcpy(buffer, &pres, sizeof(node));
            write(pres.pos);
        }
		
        tmp.sz=s-s/2;
		if (pos >= s / 2) {
			for (int i = s-s / 2; i > pos-s+s/2; i--) {
					tmp.keys[i] = tmp.keys[i - 1];
					((size_t*)tmp.data)[i+1] = ((size_t*)tmp.data)[i];
			}
			tmp.keys[pos-s+s/2] = _p.first;
			((size_t*)tmp.data)[pos-s+s/2+1] = _p.second;
            tmp.sz++;
        }
		pair<Key, size_t> pa(tmp.keys[0], tmp.pos);
        memcpy(buffer,&tmp,sizeof(node));
		memcpy(&pres, &tmp, sizeof(node));
        write(p);
        info.eof+=bufferSize;
        //this function doesn't change parent node
        return pa;
    }
    pair<Key,size_t> insertLeaf(const Value &v,const Key &k,size_t n,short pos){
        //split a node at the half of it
        size_t next,p,s=L;
        read(n);
		node tmp;
		next = pres.next;
        memcpy(&tmp,buffer,sizeof(node));
        pres.sz=s/2;
        f.seekg(0,std::ios::end);
        p=f.tellg();
        tmp.pos=pres.next=p;//A->B
		for (int i = 0; i < s - s / 2; i++) tmp.keys[i] = tmp.keys[i + s / 2];
		for (int i = 0; i < s - s / 2; i++) ((Value*)tmp.data)[i] = ((Value*)tmp.data)[i + s / 2];
		if (pos < s / 2) {
			for (int i = s/2; i >pos; i--) {
					pres.keys[i] = pres.keys[i - 1];
					((Value*)pres.data)[i] = ((Value*)pres.data)[i - 1];
			}
			pres.keys[pos] = k;
			((Value*)pres.data)[pos] = v;
			pres.sz++;
		}
		memcpy(buffer, &pres, sizeof(node));
        write(pres.pos); 
		tmp.prev=n;
        if(next){
            read(next);
            tmp.next=pres.pos;
			pres.prev = p;
			memcpy(buffer, &pres, sizeof(node));
            write(pres.pos);
        }
		
        tmp.sz=s - s/2;   
		if (pos >= s / 2) {
			for (int i = s - s / 2; i > pos-s/2; i--) {
					tmp.keys[i] = tmp.keys[i - 1];
					((Value*)tmp.data)[i] = ((Value*)tmp.data)[i - 1];
			}
			tmp.keys[pos-s/2] = k;
			((Value*)tmp.data)[pos-s/2] = v;
			tmp.sz++;
		}
		pair<Key, size_t> pa(tmp.keys[0], tmp.pos);
        memcpy(buffer,&tmp,sizeof(node));
		memcpy(&pres, buffer, sizeof(node));
		info.eof += bufferSize;
        write();
		if (info.tail == n)info.tail = p;
        //this function doesn't change parent node
        return pa;
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
        memcpy(buffer,&pres,sizeof(node));
        write(pres.pos);
        }
        //this function doesn't change parent node
    }
    iterator _find(const Key &key, bool &flag){
        short l,r,mid,ans=0;
		bool flag2 = true;
        read(info.root);
        size_t pos=info.root;
        while(pres.nodeType){
			if (key >= pres.keys[pres.sz - 1])
			{
					pos = *((size_t*)pres.data + pres.sz);
					flag2 = false;
					read(pos);
					continue;
			}
			l=0;
			r=pres.sz-1;
			mid = (l + r) / 2;
			while(l<r){
					mid=(l+r)/2;
					if(pres.keys[mid]>key)
						if (mid == 0 || pres.keys[mid - 1] <= key)break; 
						else r=mid-1;
					else if (pres.keys[mid + 1] > key) {
						mid++;
						break;
					}
					else l=mid+1;
			}
			if (l == r)mid = l;
			pos=*((size_t *)pres.data+mid);
			read(pos);
        }
        l=0;
        r=pres.sz-1;
		while (l < r)
        {
			mid = (l + r) / 2;
            if (pres.keys[mid] > key)
				r = mid - 1;
			else if (pres.keys[mid] < key)
				l = mid + 1;
			else if (pres.keys[mid] == key)
			{
				l = mid;
				break;
			}
        }
        if(pres.keys[l]==key)flag=1;//找到了
		else {
			while (pres.keys[l] < key&&l<pres.sz)l++;
			flag = 0; 
		}
        iterator it(pres.pos,pres.sz,l,this);
        return it;
    }
    void build(){
        //initialize a Btree;
        info.size = 0;
        info.head = 0;
        info.eof = sizeof(dataInfo);
        node root;
        info.head=info.tail=info.root = root.pos=info.eof;
        info.eof += bufferSize;
        root.nodeType = '\0';
        open();
        initialize();
        memcpy(buffer,&root,sizeof(node)); 
        write();
    }
public:
    typedef pair<const Key, Value> value_type;
    class const_iterator;
    class iterator
    {
        friend class BTree;
    private:
        // Your private members go here
        size_t blockPos;
        short blockSize;
        short dataPos;
        BTree  *tree;
    public:
        bool modify(const Value &value)
        {
            return false;
        }
        iterator()
        {
            blockPos=dataPos=0;
            // TODO Default Constructor
        }
        iterator(size_t b,size_t s,short d, BTree *t):blockPos(b),blockSize(s),dataPos(d),tree(t){}
        iterator(const iterator &other)
        {
            blockPos = other.blockPos;
            dataPos=other.dataPos;
            blockSize=other.blockSize;
            // TODO Copy Constructor
        }
        // Return a new iterator which points to the n-next elements
        iterator operator++(int)
        {  
            if(blockPos+dataPos==tree->info.eof)
                throw index_out_of_bound();
            iterator tmp=*this;
            tree->read(blockPos);
            if(dataPos==tree->pres.sz-1){
                tree->read(pres.next);//read next node
                blockPos =tree->pres.pos;
                dataPos = 0;
                blockSize=tree->pres.sz;
            }
            else
                dataPos++;
            tree->close();
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
                throw index_out_of_bound();
            iterator tmp=*this;
            tree->read(blockPos);
            if(dataPos==0){
                read(tree->pres.prev);//read next node
                blockPos = tree->pres.pos;
                blockSize = tree->pres.sz;
                dataPos = tree->pres.sz - 1;
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
                throw index_out_of_bound();
            iterator tmp=*this;
            tree->read(blockPos);
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
        friend class BTree;
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
            blockPos = other.blockPos;
            blockSize=pres.sz;
            dataPos=other.dataPos;
        }
        const_iterator(const iterator &other)
        {
            blockPos = other.blockPos;
            dataPos=other.dataPos;
        }
        const_iterator operator++(int)
        {  
            if(blockPos+dataPos==tree->info.eof)
                throw index_out_of_bound();
            const_iterator tmp=*this;
            tree->read(blockPos);
            if (dataPos == tree->pres.sz - 1)
            {
                tree->read(tree->pres.next); //read next node
                blockPos = tree->pres.pos;
                blockSize = tree->pres.sz;
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
                throw index_out_of_bound();
            const_iterator tmp=*this;
            tree->read(blockPos);
            if(dataPos==0){
                read(tree->pres.prev); //read next node
                blockPos = tree->pres.pos;
                blockSize = tree->pres.sz;
                dataPos = tree->pres.sz - 1;
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
        char names[7] = "0_tree";
        setName(names);
        ID++;
        buffer=new char[bufferSize];
        build();
    }
    BTree(char *s){//预设文件名的构造函数
        setName(s);
        buffer=new char[bufferSize];
        build();
    }
    BTree(const BTree &other)
    {
        char names[7] = "0_tree";
        setName(names);
        ID++;
        buffer=new char[bufferSize];
        open();
        copy(other);
    }
    BTree &operator=(const BTree &other)
    {
        char names[7] = "0_tree";
        setName(names);
        ID++;
        buffer=new char[bufferSize];
        open();
        copy(other);
        return *this;
        // Todo Assignment
    }
    ~BTree()
    {
        close();
        delete[] buffer;
    }
    // Insert: Insert certain Key-Value into the database
    // Return a pair, the first of the pair is the iterator point to the new
    // element, the second of the pair is Success if it is successfully inserted
    pair<iterator, OperationResult> insert(const Key &key, const Value &value)
    {  
        if(info.size==0){
			info.size++; 
			read(info.root);
            pres.sz=1;
            pres.keys[0]=key;
            *((Value *)pres.data)=value;
            info.tail=info.head=pres.pos;
			memcpy(buffer, &pres, sizeof(node));
            write(pres.pos);
            iterator it=begin();
            pair<iterator,OperationResult> u(it,Success);
            return u;
        }//tree is empty
        else {
            info.size++;
            bool flag;
            size_t pos;
            iterator ins=_find(key,flag);//get the right position to insert
            if(!flag){
                //avoid inserting an element with the same key
                if(ins.blockSize<L){
                     //node is availble
					for (int i = pres.sz; i > ins.dataPos; i--) {
						pres.keys[i] = pres.keys[i - 1];
						((Value*)pres.data)[i] = ((Value*)pres.data)[i - 1];
					}
					pres.keys[ins.dataPos] = key;
					((Value*)pres.data)[ins.dataPos] = value;
                    pres.sz++;
                    memcpy(buffer,&pres,sizeof(node));
                    write(pres.pos);
                    pair<iterator, OperationResult> p(ins, Success);
					return p;
                }
                else{//need to split leaf
                    size_t par=pres.parent;
					pair<Key, size_t> p=insertLeaf(value,key,ins.blockPos,ins.dataPos);
					if (par) { 
						insertIdx(p, par); 
					}
					else{
						node tmp;
						tmp.nodeType = '0';
						tmp.sz = 1;
						f.seekg(0, std::ios::end);
						tmp.pos = f.tellg();
						tmp.prev = tmp.next = tmp.parent = 0;
						tmp.keys[0] = p.first;
						*((size_t*)tmp.data) =ins.blockPos;
						*((size_t*)tmp.data + 1) = p.second;
						info.root = tmp.pos;
						memcpy(buffer, &tmp, sizeof(node));
						info.eof += bufferSize;
						write();
						read(ins.blockPos);
						pres.parent = tmp.pos;
						memcpy(buffer, &pres, sizeof(node));
						write(ins.blockPos);
						read(p.second);
						pres.parent = tmp.pos;
						memcpy(buffer, &pres, sizeof(node));
						write(p.second);
					}
                    pair<iterator,OperationResult> _p(ins,Success);
                    return _p;
                }
            }
            else {
                pair<iterator,OperationResult> p(ins,Fail);
                return p;
            }   
        }
       
    }
    // Erase: Erase the Key-Value
    // Return Success if it is successfully erased
    // Return Fail if the key doesn't exist in the database
    OperationResult erase(const Key &key)
    {
        if(info.size==0)throw container_is_empty();
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
        bool flag;
        iterator it=_find(key,flag);
		//if (!flag)
			//throw index_out_of_bound();
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
		if (flag)return it;
        else return end();
    }
};
} // namespace sjtu
