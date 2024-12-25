#ifndef DEAP_H_INCLUDED
#define DEAP_H_INCLUDED

# include <stdio.h>

/*
operator overloading :
bool operator < ( Type data1, Type data2 ) ;
*/

template<class Type>
class Deap {

private:

  Type* mHeap ;
  int mMaxSize ;
  int mHeapSize ;

  void InsertData( int index, Type data ) ;
  void ReHeapUp( int index, bool isMin ) ;
  void BlankHeapDown( int& index, bool isMin ) ;
  int GetCorrespondingIndex( int index ) ;
  void DataSwap( int index1, int index2 ) ;
  void AdjustSize() ;

public:

  explicit Deap(int max_size = 100) ; // constructor
  ~Deap() ; // destructor

  void Clear();

  void Insert( Type data ) ;

  bool GetMin( Type& data ) ;
  bool GetMinAndDelete( Type& data ) ;
  void DeleteMin() ;

  bool GetMax( Type& data ) ;
  bool GetMaxAndDelete( Type& data ) ;
  void DeleteMax() ;

  int GetSize() ;

} ; // class Deap

template<class Type>
Deap<Type>::Deap(int max_size) { // constructor

  mMaxSize = max_size ;
  mHeapSize = 1 ;

  mHeap = new Type[mMaxSize] ;

} // Deap<Type>::Deap()

template<class Type>
Deap<Type>::~Deap() { // destructor

  delete[] mHeap ;
  mHeap = NULL ;

} // Deap<Type>::~Deap()

template<class Type>
void Deap<Type>::Clear() {
    for (int i = 1; i < mHeapSize; ++i)
        mHeap[i].~Type();

    mHeapSize = 1;
} // Deap<Type>::Clear()

template<class Type>
void Deap<Type>::Insert( Type data ) {

  if ( mHeapSize == mMaxSize ) {
    AdjustSize() ;
  } // if

  if ( mHeapSize == 1 ) {
    mHeap[1] = data ;
  } // if
  else{
    InsertData( mHeapSize, data ) ;
  } // else  

  mHeapSize++ ;

} // Deap<Type>::Insert()

template<class Type>
void Deap<Type>::InsertData( int index, Type data ) {

  mHeap[index] = data ;

  int correspondingIndex = GetCorrespondingIndex( index ) ;

  if ( index < correspondingIndex ) { // index is in Min Heap

    if ( correspondingIndex >= mHeapSize ) { // correspondingIndex not exist      
      correspondingIndex = ( correspondingIndex - 1 ) / 2 ;
    } // if

    if ( mHeap[index] < mHeap[correspondingIndex] ) {
      ReHeapUp( index, true ) ; // Min Heap
    } // if
    else {
      DataSwap( index, correspondingIndex ) ;
      ReHeapUp( correspondingIndex, false ) ; // Max Heap
    } // else

  } // if
  else { // index is in Max Heap

    int leftChildIndex = correspondingIndex * 2 + 1 ;

    if ( leftChildIndex < mHeapSize ) { // // leftChildIndex exist

      int rightChildIndex = leftChildIndex + 1 ;

      if ( rightChildIndex < mHeapSize && mHeap[leftChildIndex] < mHeap[rightChildIndex] ) {
        correspondingIndex = rightChildIndex ;
      } // if
      else {
        correspondingIndex = leftChildIndex ;
      } // else

    } // if

    if ( mHeap[index] < mHeap[correspondingIndex] ) {
      DataSwap( index, correspondingIndex ) ;
      ReHeapUp( correspondingIndex, true ) ; // Min Heap
    } // if
    else {
      ReHeapUp( index, false ) ; // Max Heap
    } // else

  } // else

} // Deap<Type>::InsertData()

template<class Type>
bool Deap<Type>::GetMin( Type& data ) {

  if ( mHeapSize != 1 ) {
    data = mHeap[1] ;
    return true ;
  } // if
  else {
    return false ;
  } // else

} // Deap<Type>::GetMin()

template<class Type>
bool Deap<Type>::GetMinAndDelete( Type& data ) {

  if ( mHeapSize != 1 ) {
    data = mHeap[1] ;
    DeleteMin() ;
    return true ;
  } // if
  else {
    return false ;
  } // else

} // Deap<Type>::GetMinAndDelete()

template<class Type>
void Deap<Type>::DeleteMin() {
  
  if ( mHeapSize != 1 ) {
    mHeapSize-- ;
  } // if

  if ( mHeapSize != 1 ) {
    
    Type temp = mHeap[mHeapSize] ;
    int blankIndex = 1 ;

    BlankHeapDown( blankIndex, true ) ;

    if ( mHeapSize == 2 ) {
      mHeap[1] = temp ;
    } // if
    else {
      InsertData( blankIndex, temp ) ;
    } // else

  } // if

} // Deap<Type>::DeleteMin()

template<class Type>
bool Deap<Type>::GetMax( Type& data ) {

  if ( mHeapSize != 1 ) {
    data = ( mHeapSize == 2 ? mHeap[1] : mHeap[2] ) ;
    return true ;
  } // if
  else {
    return false ;
  } // else

} // Deap<Type>::GetMax()

template<class Type>
bool Deap<Type>::GetMaxAndDelete( Type& data ) {

  if ( mHeapSize != 1 ) {
    data = ( mHeapSize == 2 ? mHeap[1] : mHeap[2] ) ;
    DeleteMax() ;
    return true ;
  } // if
  else {
    return false ;
  } // else

} // Deap<Type>::GetMaxAndDelete()

template<class Type>
void Deap<Type>::DeleteMax() {

  if ( mHeapSize != 1 ) {
    mHeapSize-- ;
  } // if

  if ( mHeapSize != 1 && mHeapSize != 2 ) {

    Type temp = mHeap[mHeapSize] ;
    int blankIndex = 2 ;

    BlankHeapDown( blankIndex, false ) ;

    InsertData( blankIndex, temp ) ;

  } // if

} // Deap<Type>::DeleteMax()

template<class Type>
int Deap<Type>::GetSize() {

  return mHeapSize ;

} // Deap<Type>::GetSize()

template<class Type>
void Deap<Type>::AdjustSize() {

  Type* temp = mHeap ;

  mMaxSize = mMaxSize * 2 ;
  mHeap = new Type[mMaxSize] ;

  for ( int i = 0 ; i < mHeapSize ; i++ ) {
    mHeap[i] = temp[i] ;
  } // for 

  delete[] temp ;
  temp = NULL ;

} // Deap<Type>::AdjustSize()

template<class Type>
void Deap<Type>::ReHeapUp( int index, bool isMin ) {

  int currentIndex = index, parentIndex = 0 ;
  bool running = true ;

  while ( running ) {

    parentIndex = ( currentIndex - 1 ) / 2 ;

    if ( parentIndex == 0 ) {
      running = false ;
    } // if
    else {

      if ( isMin ? mHeap[currentIndex] < mHeap[parentIndex] : mHeap[parentIndex] < mHeap[currentIndex] ) {
        DataSwap( currentIndex, parentIndex ) ;
        currentIndex = parentIndex ;
      } // if
      else {
        running = false ;
      } // else      

    } // else

  } // while

} // Deap<Type>::ReHeapUp()

template<class Type>
void Deap<Type>::BlankHeapDown( int& index, bool isMin ) {

  int leftChildIndex = 0, rightChildIndex = 0 ;
  int targetIndex = 0 ;

  leftChildIndex = index * 2 + 1 ;

  while ( leftChildIndex < mHeapSize ) {

    targetIndex = leftChildIndex ;
    rightChildIndex = leftChildIndex + 1 ;

    if ( rightChildIndex < mHeapSize && ( isMin ? mHeap[rightChildIndex] < mHeap[targetIndex] :
                                                  mHeap[targetIndex] < mHeap[rightChildIndex] ) ) {
      targetIndex = rightChildIndex ;
    } // if

    mHeap[index] = mHeap[targetIndex] ;

    index = targetIndex ;
    leftChildIndex = index * 2 + 1 ;

  } // while

} // Deap<Type>::BlankHeapDown()

template<class Type>
int Deap<Type>::GetCorrespondingIndex( int index ) {

  int rightMostIndex = 0, increaseRange = 2 ;

  while ( index > rightMostIndex ) {
    rightMostIndex += increaseRange ;
    increaseRange *= 2 ;
  } // while

  int middleLeftIndex = ( rightMostIndex + ( rightMostIndex / 2 ) ) / 2 ;

  if ( index <= middleLeftIndex ) {
    return rightMostIndex - ( middleLeftIndex - index ) ;
  } // if
  else {
    return middleLeftIndex - ( rightMostIndex - index ) ;
  } // else

} // Deap<Type>::GetCorrespondingIndex()

template<class Type>
void Deap<Type>::DataSwap( int index1, int index2 ) {

  Type temp = mHeap[index1] ;
  mHeap[index1] = mHeap[index2] ;
  mHeap[index2] = temp ;

} // Deap<Type>::DataSwap()

#endif // DEAP_H_INCLUDED

