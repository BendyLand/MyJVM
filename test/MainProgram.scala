package example

object MainProgram {
  def main(args: Array[String]): Unit = {
    val student = new Student("Charlie", 19, "Physics")
    student.greet()
    student.study()
  }
}
