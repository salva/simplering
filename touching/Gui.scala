package org.vesbot.simplering.touching

import org.gnome.gdk.Event
import org.gnome.gtk.{Gtk, Widget, Window, VBox, HBox, Button, DrawingArea, HScale, Range}
import org.freedesktop.cairo.{Context}
import scala.math.{min, Pi}

object Gui {
 
  def main(args: Array[String]) = {

    var radius = 1.0
    var n = 10
    var circles = List[Circle]()
    var touch = false
 
    Gtk.init(args)
 
    val window = new Window
    val layout1 = new VBox(false, 3)
    val layout2 = new HBox(false, 3)
    val layout3 = new VBox(false, 3)
    val drawing = new DrawingArea()
    val go_button = new Button("Go")
    val radius_scale = new HScale(1, 1000, 1)
    val n_scale = new HScale(1, 100, 1)

    window.setDefaultSize(150, 150)
    window.add(layout1)
    layout1.packStart(drawing, true, true, 0)
    layout1.packStart(layout2, false, true, 0)
    layout2.packStart(layout3, true, true, 0)
    layout3.packStart(radius_scale, true, true, 0)
    layout3.packStart(n_scale, true, true, 0)
    layout2.packStart(go_button, false, true, 0)

    radius_scale.setValue((radius * 1000).toInt)
    n_scale.setValue(n)

    window.showAll()
    generate()

    def generate() : Unit = {
      circles = Circle.randomCircles(n, radius)
      recalc()
    }

    def recalc() : Unit = {
      var ops = 0;
      val c = circles.map(x => new Circle(x.center, x.radius) {
        override def touches(other: Circle) = {
          ops += 1
          if (super.touches(other)) {
            println("circles touch: " + this + ", " + other)
            true
          }
          else
            false
        }})

      touch = Circle.circlesTouch(c)
      window.setTitle((if (touch) "Touch" else "Don't touch") + " [ops: " + ops + "]")
      drawing.queueDraw()
    }
 
    radius_scale.connect( new Range.ValueChanged {
      def onValueChanged(source: Range) {
        val old_radius = radius
        radius = 0.001 * source.getValue
        if (old_radius > 0 && old_radius != radius) {
          val scale = radius / old_radius
          circles = circles.map(c => new Circle(c.center, c.radius * scale))
          recalc()
        }
      }})

    n_scale.connect( new Range.ValueChanged {
      def onValueChanged(source: Range) {
        val old_n = n
        n = source.getValue.toInt
        if (old_n != n) {
          if (old_n > n)
            circles = circles.take(n)
          else
            circles = circles ++ Circle.randomCircles(n - old_n, radius)
          recalc()
        }
      }})

    window.connect( new Window.DeleteEvent {
      def onDeleteEvent(w: Widget, e: Event) = {
        Gtk.mainQuit
        false
      }})
 
    go_button.connect( new Button.Clicked() {
      def onClicked(b:Button) = {
        generate()
      }})

    drawing.connect( new Widget.Draw {
      def onDraw(w: Widget, cr: Context) = {
        val allocation = w.getAllocation
        val scale = min(allocation.getWidth, allocation.getHeight)
        cr.scale(scale, scale)
        cr.translate(0.5, 0.5)
        cr.scale(0.5, 0.5)
        cr.setSource(1.0, 1.0, 1.0)
        cr.paint()
        if (touch) 
          cr.setSource(1.0, 0.1, 0.0, 1.0)
        else
          cr.setSource(0.1, 1.0, 0.0, 1.0)
        cr.setLineWidth(0.01)
        circles.foreach(c => {
          cr.arc(c.center.x, c.center.y, c.radius, 0, 2 * Pi)
          cr.stroke();
        })
        true
      }})

    Gtk.main

  }
}
