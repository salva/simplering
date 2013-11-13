
import scala.language.implicitConversions

class ArrayBitSortOp[T](val a: Array[T]) {
  private def reverse(start: Int, len: Int) {
    var i = start;
    var j = start + len - 1;
    while (i < j) {
      val tmp = a(i);
      a(i) = a(j);
      a(j) = tmp;
      i += 1;
      j -= 1;
    }
  }

  private def shift[A](start: Int, len1: Int, len2: Int) {
    reverse(start, len1);
    reverse(start + len1, len2);
    reverse(start, len1 + len2);
  }

  private def mergeSort(bit: T => Boolean, start: Int, len: Int) : Int = {
    if (len > 1) {
      val len1 = len / 2;
      val mid1 = mergeSort(bit, start, len1);
      val mid2 = mergeSort(bit, start + len1, len - len1);
      shift(start + mid1, len1 - mid1, mid2);
      mid1 + mid2;
    }
    else if ((len == 0) || bit(a(start)))
      0
    else
      1
  }

  def bitSort[A](bit: T => Boolean) {
    mergeSort(bit, 0, a.length);
  }
}

implicit def arrayToArrayBitSortOp[T](a: Array[T]) = new ArrayBitSortOp(a)


// Usage:
//
//   def isDigit(x: Char) = x >= '0' && x <= '9'
//   val a = Array('1', '2', 'a', '3', 'b', 'c', '4', '5', 'd', 'e')
//   a.bitSort(isDigit)
//   println(a.mkString(", "))
