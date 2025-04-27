public class Student extends Person {
    private String major;

    public Student(String name, int age, String major) {
        super(name, age);
        this.major = major;
    }

    @Override
    public void introduce() {
        super.introduce();
        System.out.println("I'm studying " + major + ".");
    }
}
