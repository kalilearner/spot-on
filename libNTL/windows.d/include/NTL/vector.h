
#ifndef NTL_vector__H
#define NTL_vector__H

#include <NTL/tools.h>
#include <new>

struct _ntl_VectorHeader {
   long length;
   long alloc;
   long init;
   long fixed;
};

union _ntl_AlignedVectorHeader {
   _ntl_VectorHeader h;
   double x1;
   long x2;
   char *x3;
   long double x4;
};

#define NTL_VECTOR_HEADER_SIZE (sizeof(_ntl_AlignedVectorHeader))

#define NTL_VEC_HEAD(p) (& (((_ntl_AlignedVectorHeader *) (p.rep))[-1].h))


#ifndef NTL_RANGE_CHECK
#define NTL_RANGE_CHECK_CODE(i) 
#else
#define NTL_RANGE_CHECK_CODE(i) if ((i) < 0 || !_vec__rep || (i) >= NTL_VEC_HEAD(_vec__rep)->length) LogicError("index out of range in Vec");

#endif

// vectors are allocated in chunks of this size

#ifndef NTL_VectorMinAlloc
#define NTL_VectorMinAlloc (4)
#endif

// vectors are always expanded by at least this ratio

#ifndef NTL_VectorExpansionRatio
#define NTL_VectorExpansionRatio (1.2)
#endif

// controls initialization during input

#ifndef NTL_VectorInputBlock
#define NTL_VectorInputBlock 50
#endif


NTL_OPEN_NNS

  



template<class T>
void BlockDestroy(T* p, long n)  
{  
   for (long i = 0; i < n; i++)  
      p[i].~T();  

   // NOTE: this routine is only invoked through a Vec destructor
   // or a scope guard destructor, both of which are noexcept destructors.
   // therefore, if ~T() should throw, the program will terminate
}


template<class T>
void BlockConstruct(T* p, long n)  
{  
   long i;

   NTL_SCOPE(guard) { BlockDestroy(p, i); };

   for (i = 0; i < n; i++)  
      (void) new(&p[i]) T;  

   guard.relax();

   // NOTE: we invoke T rather than T(), which would ensure
   // POD types get zeroed out, but only in compilers that
   // comply with C++03, which does not include MS compilers.
   // So we just use T, which is less expensive, and it is better
   // not to assume POD types get initialized.
   
}  

template<class T>
void BlockConstructFromVec(T* p, long n, const T* q)  
{  
   long i;

   NTL_SCOPE(guard) { BlockDestroy(p, i); };

   for (i = 0; i < n; i++)  
      (void) new(&p[i]) T(q[i]);  

   guard.relax();
}  

template<class T>
void BlockConstructFromObj(T* p, long n, const T& q)  
{  
   long i;

   NTL_SCOPE(guard) { BlockDestroy(p, i); };

   for (i = 0; i < n; i++)  
      (void) new(&p[i]) T(q);  

   guard.relax();
}  

// experimental stuff to implement Vec<T> for non-relocatable T
// Right now, it is not used anywhere


struct RelocatableTypeTag { };
struct NotRelocatableTypeTag { };

template<class T>
RelocatableTypeTag IsRelocatableType(T*) { return RelocatableTypeTag(); }


template<class T>
class Vec {  
public:  

   static bool relocatable(RelocatableTypeTag) { return true; }
   static bool relocatable(NotRelocatableTypeTag) { return false; }
   static bool relocatable() { return relocatable(IsRelocatableType((T*)0)); }

   class _vec_deleter {
   public:
      static void apply(T* p) { 
         if (p)  {
            NTL_SNS free(((char *) p) - sizeof(_ntl_AlignedVectorHeader));
         }
      }
   };

   WrappedPtr<T, _vec_deleter> _vec__rep;  
  
  
   Vec() { }  

   Vec(INIT_SIZE_TYPE, long n) { SetLength(n); }  
   Vec(INIT_SIZE_TYPE, long n, const T& a) { SetLength(n, a); }  

   // the following copy constructor does not rely on
   // the assignment operator
   Vec(const Vec<T>& a)  
   {  
      long src_len = a.length();
      const T *src = a.elts();
      AllocateTo(src_len);
      Init(src_len, src);
      AdjustLength(src_len);
   }     

   Vec<T>& operator=(const Vec<T>& a);  

   ~Vec();  
   void kill(); 
  
   void SetMaxLength(long n); 
   void FixLength(long n); 
   void FixAtCurrentLength();
   void QuickSetLength(long n) { NTL_VEC_HEAD(_vec__rep)->length = n; } 

   void SetLength(long n) {
      if (_vec__rep && !NTL_VEC_HEAD(_vec__rep)->fixed &&
          n >= 0 && n <= NTL_VEC_HEAD(_vec__rep)->init)
         NTL_VEC_HEAD(_vec__rep)->length = n;
      else 
         DoSetLength(n);
   }

   void SetLength(long n, const T& a) {
      if (_vec__rep && !NTL_VEC_HEAD(_vec__rep)->fixed &&
          n >= 0 && n <= NTL_VEC_HEAD(_vec__rep)->init)
         NTL_VEC_HEAD(_vec__rep)->length = n;
      else 
         DoSetLength(n, a);
   }

   template<class F>
   void SetLengthAndApply(long n, F f) {
      if (_vec__rep && !NTL_VEC_HEAD(_vec__rep)->fixed &&
          n >= 0 && n <= NTL_VEC_HEAD(_vec__rep)->init)
         NTL_VEC_HEAD(_vec__rep)->length = n;
      else 
         DoSetLengthAndApply(n, f);
   }



   long length() const 
   { return (!_vec__rep) ?  0 : NTL_VEC_HEAD(_vec__rep)->length; }  

   long MaxLength() const 
   { return (!_vec__rep) ?  0 : NTL_VEC_HEAD(_vec__rep)->init; } 

   long allocated() const 
   { return (!_vec__rep) ?  0 : NTL_VEC_HEAD(_vec__rep)->alloc; } 

   long fixed() const 
   { return _vec__rep && NTL_VEC_HEAD(_vec__rep)->fixed; } 
  
   T& operator[](long i)   
   {  
      NTL_RANGE_CHECK_CODE(i)  
      return _vec__rep[i];  
   }  
  
   const T& operator[](long i) const 
   {  
      NTL_RANGE_CHECK_CODE(i)  
      return _vec__rep[i];  
   }  
  
   T& RawGet(long i)   
   {  
      return _vec__rep[i];  
   }  
  
   const T& RawGet(long i) const 
   {  
      return _vec__rep[i];  
   }  
  
   T& operator()(long i) { return (*this)[i-1]; }  
   const T& operator()(long i) const { return (*this)[i-1]; } 
   
  
   const T* elts() const { return _vec__rep; }  
   T* elts() { return _vec__rep; }  
         
   Vec(Vec<T>& x, INIT_TRANS_TYPE) 
   { _vec__rep.swap(x._vec__rep); }

   long position(const T& a) const;  
   long position1(const T& a) const;  

   void swap(Vec<T>& y);
   void append(const T& a);
   void append(const Vec<T>& w);


// Some compatibility with vec_GF2

   const T& get(long i) const 
   { return (*this)[i]; }
 
   void put(long i, const T& a)
   { (*this)[i] = a; }


// Some STL compatibility

   typedef T value_type;
   typedef value_type& reference;
   typedef const value_type& const_reference;
   typedef value_type *iterator;
   typedef const value_type *const_iterator; 

   const T* data() const { return elts(); }
   T* data() { return elts(); }

   T* begin() { return elts(); }
   const T* begin() const { return elts(); }

   T* end() { 
      if (elts()) 
         return elts() + length(); 
      else
         return 0;
   }

   const T* end() const { 
      if (elts()) 
         return elts() + length(); 
      else
         return 0;
   }

   T& at(long i) {
      if ((i) < 0 || !_vec__rep || (i) >= NTL_VEC_HEAD(_vec__rep)->length)  
         LogicError("index out of range in Vec");
      return _vec__rep[i];  
   }

   const T& at(long i) const {
      if ((i) < 0 || !_vec__rep || (i) >= NTL_VEC_HEAD(_vec__rep)->length)  
         LogicError("index out of range in Vec");
      return _vec__rep[i];  
   }

   class Watcher {
   public:
      Vec& watched;
      explicit
      Watcher(Vec& _watched) : watched(_watched) {}

      ~Watcher() 
      { 
         if (watched.MaxLength() > NTL_RELEASE_THRESH) watched.kill();
      }
   };

private:
   void DoSetLength(long n);
   void DoSetLength(long n, const T& a);

   template<class F>
   void DoSetLengthAndApply(long n, F& f);

   void AdjustLength(long n) { if (_vec__rep) NTL_VEC_HEAD(_vec__rep)->length = n; }
   void AdjustAlloc(long n) { if (_vec__rep) NTL_VEC_HEAD(_vec__rep)->alloc = n; }
   void AdjustMaxLength(long n) { if (_vec__rep) NTL_VEC_HEAD(_vec__rep)->init = n; }

   void ReAllocate(long n, RelocatableTypeTag);
   void ReAllocate(long n, NotRelocatableTypeTag);

   void AllocateTo(long n); // reserves space for n items
   void Init(long n); // make sure first n entries are initialized
   void Init(long n, const T* src); // same, but use src
   void Init(long n, const T& src); // same, but use src

   template<class F>
   void InitAndApply(long n, F& f);
};  
 



#if (!defined(NTL_CLEAN_PTR))

template<class T>
long Vec<T>::position(const T& a) const  
{  
   if (!_vec__rep) return -1;  
   long num_alloc = NTL_VEC_HEAD(_vec__rep)->alloc;  
   long num_init = NTL_VEC_HEAD(_vec__rep)->init;  
   if (&a < _vec__rep || &a >= _vec__rep + num_alloc) return -1;  
   long res = (&a) - _vec__rep;  
   
   if (res < 0 || res >= num_alloc ||   
       _vec__rep + res != &a) return -1;  
   
   if (res >= num_init)  
       LogicError("position: reference to uninitialized object"); 
   return res;  
}  
  
template<class T>
long Vec<T>::position1(const T& a) const  
{  
   if (!_vec__rep) return -1;  
   long len = NTL_VEC_HEAD(_vec__rep)->length;  
   if (&a < _vec__rep || &a >= _vec__rep + len) return -1;  
   long res = (&a) - _vec__rep;  
   
   if (res < 0 || res >= len ||   
       _vec__rep + res != &a) return -1;  
   
   return res;  
}  


#else

template<class T>
long Vec<T>::position(const T& a) const  
{  
   if (!_vec__rep) return -1;  
   long num_alloc = NTL_VEC_HEAD(_vec__rep)->alloc;  
   long num_init = NTL_VEC_HEAD(_vec__rep)->init;  
   long res;  
   res = 0;  
   while (res < num_alloc && _vec__rep + res != &a)  res++;  
   if (res >= num_alloc) return -1;  
   if (res >= num_init)  
       LogicError("position: reference to uninitialized object"); 
   return res;  
}  
 
template<class T>
long Vec<T>::position1(const T& a) const  
{  
   if (!_vec__rep) return -1;  
   long len = NTL_VEC_HEAD(_vec__rep)->length;  
   long res;  
   res = 0;  
   while (res < len && _vec__rep + res != &a)  res++;  
   if (res >= len) return -1;  
   return res;  
}  


#endif

template<class T>
void Vec<T>::ReAllocate(long m, RelocatableTypeTag)   
{
   char *p = ((char *) _vec__rep.rep) - sizeof(_ntl_AlignedVectorHeader); 
   p = (char *) NTL_SNS_REALLOC(p, m, sizeof(T), sizeof(_ntl_AlignedVectorHeader)); 
   if (!p) {  
      MemoryError();  
   }  
   _vec__rep = (T *) (p + sizeof(_ntl_AlignedVectorHeader)); 
   NTL_VEC_HEAD(_vec__rep)->alloc = m;  
}

template<class T>
void Vec<T>::ReAllocate(long m, NotRelocatableTypeTag)   
{
   Vec<T> tmp;
   long src_len = length();
   long src_init = MaxLength();
   const T *src = elts();

   char *p = (char *) NTL_SNS_MALLOC(m, sizeof(T), sizeof(_ntl_AlignedVectorHeader)); 
   if (!p) {  
      MemoryError();  
   }  
   tmp._vec__rep = (T *) (p + sizeof(_ntl_AlignedVectorHeader)); 

   NTL_VEC_HEAD(tmp._vec__rep)->length = 0;  
   NTL_VEC_HEAD(tmp._vec__rep)->alloc = m;  
   NTL_VEC_HEAD(tmp._vec__rep)->init = 0;  
   NTL_VEC_HEAD(tmp._vec__rep)->fixed = 0;  

   tmp.Init(src_init, src);    // copy all initialized elements
   tmp.AdjustLength(src_len);
   tmp.swap(*this);
}
 
template<class T>
void Vec<T>::AllocateTo(long n)   
{   
   long m;  
  
   if (n < 0) {  
      LogicError("negative length in vector::SetLength");  
   }  
   if (NTL_OVERFLOW(n, sizeof(T), 0))  
      ResourceError("excessive length in vector::SetLength"); 
      
   if (_vec__rep && NTL_VEC_HEAD(_vec__rep)->fixed) {
      if (NTL_VEC_HEAD(_vec__rep)->length == n) 
         return; 
      else 
         LogicError("SetLength: can't change this vector's length"); 
   }  

   if (n == 0) {  
      return;  
   }  
  
   if (!_vec__rep) {  
      m = ((n+NTL_VectorMinAlloc-1)/NTL_VectorMinAlloc) * NTL_VectorMinAlloc; 
      char *p = (char *) NTL_SNS_MALLOC(m, sizeof(T), sizeof(_ntl_AlignedVectorHeader)); 
      if (!p) {  
	 MemoryError();  
      }  
      _vec__rep = (T *) (p + sizeof(_ntl_AlignedVectorHeader)); 
  
      NTL_VEC_HEAD(_vec__rep)->length = 0;  
      NTL_VEC_HEAD(_vec__rep)->alloc = m;  
      NTL_VEC_HEAD(_vec__rep)->init = 0;  
      NTL_VEC_HEAD(_vec__rep)->fixed = 0;  
   }  
   else if (n > NTL_VEC_HEAD(_vec__rep)->alloc) {  
      m = max(n, long(NTL_VectorExpansionRatio*NTL_VEC_HEAD(_vec__rep)->alloc));  
      m = ((m+NTL_VectorMinAlloc-1)/NTL_VectorMinAlloc) * NTL_VectorMinAlloc; 

      ReAllocate(m, IsRelocatableType((T*)0));
   }  
}  

template<class T>
void Vec<T>::Init(long n)   
{   
   long num_init = MaxLength();
   if (n <= num_init) return;

   BlockConstruct(_vec__rep + num_init, n-num_init);
   AdjustMaxLength(n);
}

template<class T>
void Vec<T>::Init(long n, const T *src) 
{
   long num_init = MaxLength();
   if (n <= num_init) return;

   BlockConstructFromVec(_vec__rep + num_init, n-num_init, src);
   AdjustMaxLength(n);

}

template<class T>
void Vec<T>::Init(long n, const T& src) 
{   
   long num_init = MaxLength();
   if (n <= num_init) return;

   BlockConstructFromObj(_vec__rep + num_init, n-num_init, src);
   AdjustMaxLength(n);
}

template<class T> template<class F>
void Vec<T>::InitAndApply(long n, F& f)   
{   
   long num_init = MaxLength();
   if (n <= num_init) return;

   BlockConstruct(_vec__rep + num_init, n-num_init);

   NTL_SCOPE(guard) { BlockDestroy(_vec__rep + num_init, n - num_init); };

   long i;
   for (i = num_init; i < n; i++)
      f(_vec__rep[i]);

   guard.relax();

   AdjustMaxLength(n);
}

template<class T>
void Vec<T>::DoSetLength(long n)
{
   AllocateTo(n);
   Init(n);
   AdjustLength(n);
}

template<class T>
void Vec<T>::DoSetLength(long n, const T& a)
{
   // if vector gets moved, we have to worry about
   // a aliasing a vector element
   const T *src = &a;
   long pos = -1;
   if (n > allocated()) pos = position(a);
   AllocateTo(n);
   if (pos != -1) src = elts() + pos;
   Init(n, *src);
   AdjustLength(n);
}

template<class T> template<class F>
void Vec<T>::DoSetLengthAndApply(long n, F& f)
{
   AllocateTo(n);
   InitAndApply(n, f);
   AdjustLength(n);
}


 
 
template<class T>
void Vec<T>::SetMaxLength(long n) 
{ 
   long OldLength = length(); 
   SetLength(n); 
   SetLength(OldLength); 
} 
 
template<class T>
void Vec<T>::FixLength(long n) 
{ 
   if (_vec__rep) LogicError("FixLength: can't fix this vector"); 
   if (n < 0) LogicError("FixLength: negative length"); 

   NTL_SCOPE(guard) { _vec__rep.kill(); };

   if (n > 0) 
      SetLength(n); 
   else { 
      char *p = (char *) NTL_SNS_MALLOC(0, sizeof(T), sizeof(_ntl_AlignedVectorHeader)); 
      if (!p) {  
	 MemoryError();  
      }  
      _vec__rep = (T *) (p + sizeof(_ntl_AlignedVectorHeader)); 
  
      NTL_VEC_HEAD(_vec__rep)->length = 0;  
      NTL_VEC_HEAD(_vec__rep)->init = 0;  
      NTL_VEC_HEAD(_vec__rep)->alloc = 0;  
   } 
   NTL_VEC_HEAD(_vec__rep)->fixed = 1; 

   guard.relax();
} 

template<class T>
void Vec<T>::FixAtCurrentLength() 
{
   if (fixed()) return;
   if (length() != MaxLength()) 
      LogicError("FixAtCurrentLength: can't fix this vector");

   if (_vec__rep)
      NTL_VEC_HEAD(_vec__rep)->fixed = 1;
   else
      FixLength(0);
}
  
template<class T>
Vec<T>& Vec<T>::operator=(const Vec<T>& a)  
{  
   if (this == &a) return *this;

   long init = MaxLength();
   long src_len = a.length();
   const T *src = a.elts();

   AllocateTo(src_len);
   T *dst = elts();


   if (src_len <= init) {
      long i;
      for (i = 0; i < src_len; i++)
         dst[i] = src[i];
   }
   else {
      long i;
      for (i = 0; i < init; i++)
         dst[i] = src[i];
      Init(src_len, src+init);
   }

   AdjustLength(src_len);

   return *this;  
}  
       
  
template<class T>
Vec<T>::~Vec()  
{  
   if (!_vec__rep) return;  
   BlockDestroy(_vec__rep.rep, NTL_VEC_HEAD(_vec__rep)->init); 
}  
   
template<class T>
void Vec<T>::kill()  
{  
   Vec<T> tmp;
   this->swap(tmp);
}  
  
  
template<class T>
void Vec<T>::swap(Vec<T>& y)  
{  
   long xf = fixed();  
   long yf = y.fixed();  
   if (xf != yf ||   
       (xf && NTL_VEC_HEAD(_vec__rep)->length != NTL_VEC_HEAD(y._vec__rep)->length))  
      LogicError("swap: can't swap these vectors");  

   _vec__rep.swap(y._vec__rep);
} 

template<class T>
void swap(Vec<T>& x, Vec<T>& y)  
{ 
   x.swap(y);
}
 
// EXCEPTIONS: provides strong ES
template<class T>
void Vec<T>::append(const T& a)  
{  
   long len = length();
   long init = MaxLength();
   long src_len = 1;

   // if vector gets moved, we have to worry about
   // a aliasing a vector element
   const T *src = &a;
   long pos = -1;
   if (len >= allocated()) pos = position(a);  
   AllocateTo(len+src_len);

   // The logic here is copy-pasted from the append-vector
   // logic...mostly

   long i;
   T *dst = elts();
   if (pos != -1) src = dst + pos;

   if (len+src_len <= init) {
      for (i = 0; i < src_len; i++)
         dst[i+len] = src[i];
   }
   else {
      for (i = 0; i < init-len; i++)
         dst[i+len] = src[i];

      // make sure we use BlockConstructFromObj
      Init(src_len+len, *src);
   }

   AdjustLength(len+src_len);
}  

template<class T>
void append(Vec<T>& v, const T& a)  
{
   v.append(a);
}
  
template<class T>
void Vec<T>::append(const Vec<T>& w)  
{  
   long len = length();
   long init = MaxLength();
   long src_len = w.length();

   AllocateTo(len+src_len);
   const T *src = w.elts();
   T *dst = elts();

   if (len+src_len <= init) {
      long i;
      for (i = 0; i < src_len; i++)
         dst[i+len] = src[i];
   }
   else {
      long i;
      for (i = 0; i < init-len; i++)
         dst[i+len] = src[i];
      Init(src_len+len, src+init-len);
   }

   AdjustLength(len+src_len);
}


template<class T>
void append(Vec<T>& v, const Vec<T>& w)  
{
   v.append(w);
}


template<class T>
NTL_SNS istream & operator>>(NTL_SNS istream& s, Vec<T>& a)   
{   
   Vec<T> ibuf;  
   long c;   
   long n;   
   if (!s) NTL_INPUT_ERROR(s, "bad vector input"); 
   
   c = s.peek();  
   while (IsWhiteSpace(c)) {  
      s.get();  
      c = s.peek();  
   }  
   if (c != '[') {  
      NTL_INPUT_ERROR(s, "bad vector input");  
   }  
   
   n = 0;   
   ibuf.SetLength(0);  
      
   s.get();  
   c = s.peek();  
   while (IsWhiteSpace(c)) {  
      s.get();  
      c = s.peek();  
   }  
   while (c != ']' && !IsEOFChar(c)) {   
      if (n % NTL_VectorInputBlock == 0) ibuf.SetMaxLength(n + NTL_VectorInputBlock); 
      n++;   
      ibuf.SetLength(n);   
      if (!(s >> ibuf[n-1])) NTL_INPUT_ERROR(s, "bad vector input");   
      c = s.peek();  
      while (IsWhiteSpace(c)) {  
         s.get();  
         c = s.peek();  
      }  
   }   

   if (IsEOFChar(c)) NTL_INPUT_ERROR(s, "bad vector input");  
   s.get(); 
   
   a = ibuf; 
   return s;   
}    
   
   
template<class T>
NTL_SNS ostream& operator<<(NTL_SNS ostream& s, const Vec<T>& a)   
{   
   long i, n;   
  
   n = a.length();  
   
   s << '[';   
   
   for (i = 0; i < n; i++) {   
      s << a[i];   
      if (i < n-1) s << " ";   
   }   
   
   s << ']';   
      
   return s;   
}   

template<class T>
long operator==(const Vec<T>& a, const Vec<T>& b) 
{  
   long n = a.length();  
   if (b.length() != n) return 0;  
   const T* ap = a.elts(); 
   const T* bp = b.elts(); 
   long i;  
   for (i = 0; i < n; i++) if (ap[i] != bp[i]) return 0;  
   return 1;  
} 



template<class T>
long operator!=(const Vec<T>& a, const Vec<T>& b) 
{  return !(a == b); }




// conversions

template<class T, class S>
void conv(Vec<T>& x, const Vec<S>& a)
{
   long n = a.length();
   x.SetLength(n);

   for (long i = 0; i < n; i++)
      conv(x[i], a[i]);
}


NTL_CLOSE_NNS



   





#endif

