package example

class Student(name: String, age: Int, val major: String) extends Person(name, age) {
  def study(): Unit = {
    println(s"I am studying $major.")
  }
}
