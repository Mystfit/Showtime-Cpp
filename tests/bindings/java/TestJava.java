import showtime.showtime;

public class TestJava {
    public static void main(String[] args){
        showtime.showtime.init("javatester", true);
        showtime.showtime.join("127.0.0.1");
        showtime.showtime.destroy();
    }
}
