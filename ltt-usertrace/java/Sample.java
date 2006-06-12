// The Sample.java file
public class Sample
{
  // Declaration of the Native (C) function
  private static native void trace_java_generic_string(String arg);
  static {
    System.loadLibrary("ltt-java-string");
  }

  public static void main(String[] args)
  {
    Sample.trace_java_generic_string("Tracing from java");
  }
}
