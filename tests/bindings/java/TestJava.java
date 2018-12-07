import io.showtime.ShowtimeJava;

public class TestShowtime{
    public static void main(String[] args){
        io.showtime.ShowtimeJava.init("javatester", true);
        io.showtime.ShowtimeJava.join("127.0.0.1");
        io.showtime.ShowtimeJava.destroy();
    }
}

// "C:\Program Files\Java\jdk1.8.0_121\bin\javac.exe" -classpath "C:\Code\Showtime-Cpp\build\bin\ShowtimeJava.jar" TestShowtime.java
// "C:\Program Files\Java\jdk1.8.0_121\bin\java" -classpath C:\Code\Showtime-Cpp\build\bin\ShowtimeJava.jar;. -Djava.library.path=C:\Code\Showtime-Cpp\build\bin TestShowtime