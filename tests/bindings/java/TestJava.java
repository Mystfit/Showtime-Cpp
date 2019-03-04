import showtime.ShowtimeJava;

public class TestJava {
    public static void main(String[] args){
        showtime.ShowtimeJava.init("javatester", true);
        showtime.ShowtimeJava.join("127.0.0.1");
        showtime.ShowtimeJava.destroy();
    }
}
