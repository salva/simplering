
package org.vesbot.simplering.touching

import scala.math.floor

case class Circle (center: Vector2d, radius: Double) {
  def touches(other: Circle) = (center dist other.center) <= radius + other.radius

  override def toString = "{" + center + ", " + radius + "}"

}

object Circle {

  val hashSize = 256

  def circlesTouch(circles: List[Circle]) : Boolean = {

    def hash2d[T](x: Double, y: Double) = {
      val hash = (x, y).## % hashSize
      if (hash < 0) hashSize + hash else hash
    }

    def push[T](a: Array[List[T]], ix: Int, head: T) : Unit = {
      val current = a(ix)
      val tail = if (current == null) List[T]() else current
      a(ix) = head::tail
    }

    def touch(circles: List[Circle]) : Boolean = {
      circles match {
        // touch is only possible if there are two or more elements on
        // the list
        case bigger::(circles @ next::tail) => {
          if (circles.exists(bigger touches _)) true
          else {
            // ensure that there is at least one element on the tail
            tail match {
              case _::_ => {
                val d = next.radius * 2
                if (d > 0.0) {
                  val array = new Array[List[Circle]](hashSize)
                  for (circle <- circles) {
                    val x0 = floor((circle.center.x - circle.radius) / d)
                    val x1 = floor((circle.center.x + circle.radius) / d)
                    val y0 = floor((circle.center.y - circle.radius) / d)
                    val y1 = floor((circle.center.y + circle.radius) / d)
                    val hash00 = hash2d(x0, y0)
                    val hash01 = hash2d(x0, y1)
                    val hash10 = hash2d(x1, y0)
                    val hash11 = hash2d(x1, y1)
                    //println("d: " + d + ", xc: " + circle.center.x + ", yc: " + circle.center.y + ", r: " + circle.radius +
                    //        ", x0: " + x0 + ", y0: " + y0 + ", x1: " + x1 + ", y1: " + y1 +
                    //        ", 00: " + hash00 + ", 01: " + hash01 + ", 10: " + hash10 + ", 11: " + hash11)
                    if (true)
                      push(array, hash00, circle)
                    if (hash01 != hash00)
                      push(array, hash01, circle)
                    if (hash10 != hash00 && hash10 != hash01)
                      push(array, hash10, circle)
                    if (hash11 != hash00 && hash11 != hash01 && hash11 != hash10)
                      push(array, hash11, circle)
                  }
                  array.exists(x => x != null && touch(x.reverse))
                }
                else false
              }
              case _ => false
            }
          }
        }
        case _ => false
      }
    }

    touch(circles.sortBy(_.radius).reverse)
  }

  def circlesTouchBruteForce(circles: List[Circle]) : Boolean = {
    circles match {
      case circle::circles => circles.exists(circle touches _) || circlesTouchBruteForce(circles)
      case _               => false
    }
  }

  def randomCircles (n: Int, maxRadius: Double) = {
    import scala.math.random
      (0 until n).map(_ => Circle(Vector2d(2 * random - 1.0, 2 * random - 1.0), maxRadius * random)).toList
  }
}
