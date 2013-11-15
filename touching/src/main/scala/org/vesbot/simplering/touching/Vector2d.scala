
package org.vesbot.simplering.touching

import scala.math.sqrt

case class Vector2d (x: Double, y: Double) {

  def this(other: Vector2d) = this(other.x, other.y)

  def - (other: Vector2d) = Vector2d(x - other.x, y - other.y)

  def + (other: Vector2d) = Vector2d(x + other.x, y + other.y)

  def * (other: Vector2d) = x * other.x + y * other.y

  def * (s: Double) = Vector2d(s * x, s * y)

  def unary_- = Vector2d(-x, -y)

  def dist2(other: Vector2d) = {
    val d = this - other
    d * d
  }

  def dist (other: Vector2d) = {
    sqrt(dist2(other))
  }

  override def toString = "[" + x + ", " + y + "]"
}
