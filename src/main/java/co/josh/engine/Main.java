package co.josh.engine;

import ca.weblite.objc.Client;
import ca.weblite.objc.Proxy;
import co.josh.engine.objects.GameObject;
import co.josh.engine.render.Camera;
import co.josh.engine.render.RenderDispatcher;
import co.josh.engine.render.joshshade.JoshShaderLoader;
import co.josh.engine.render.lights.Light;
import co.josh.engine.util.Transform;
import co.josh.engine.util.annotations.hooks.*;
import co.josh.engine.util.exceptions.WindowCreateFailure;
import co.josh.engine.util.input.KeyboardHandler;
import co.josh.engine.util.input.MouseHandler;
import co.josh.engine.util.texture.ByteImage;
import co.josh.engine.util.texture.TextureLoader;
import org.joml.Vector3f;
import org.lwjgl.Version;
import org.lwjgl.glfw.*;
import org.lwjgl.opengl.GL;
import org.lwjgl.opengl.GL13;
import org.lwjgl.system.MemoryStack;
import org.reflections.Reflections;
import org.reflections.scanners.MethodAnnotationsScanner;
import org.reflections.util.ClasspathHelper;
import org.reflections.util.ConfigurationBuilder;
import co.josh.engine.components.Component;

import java.io.IOException;
import java.lang.annotation.Annotation;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.IntBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Objects;
import java.util.Set;

import static org.lwjgl.glfw.Callbacks.glfwFreeCallbacks;
import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL13.*;
import static org.lwjgl.system.MemoryStack.stackPush;
import static org.lwjgl.system.MemoryUtil.NULL;

public class Main {

    public static boolean runEvents = false;

    public static String gameFolder = "/josh";

    public static int width;
    public static int height;

    public static ArrayList<Light> lights = new ArrayList<>();

    public static float targetFps = 60f;

    public static float frameWait = (int)((1f/targetFps)*1000);
    public static float tickDeltaTime = 0f;
    public static float deltaTime = 0f;

    public static float[] ambient = {0.25f, 0.25f, 0.25f, 1f};

    public static void recalcFrameWait(){
        frameWait = (int)((1f/targetFps)*1000);
    }

    public static float targetTps = 30f;

    public static float tickWait = (int)((1f/targetTps)*1000);

    public static float actualTickWait = tickWait;

    public static void recalcTickWait(){
        tickWait = (int)((1f/targetTps)*1000);
    }

    public static Camera camera;

    public static int currentWidth;
    public static int currentHeight;

    public static ArrayList<GameObject> gameObjects = new ArrayList<>();

    public static long window;

    public static KeyboardHandler keyboard;
    public static MouseHandler mouse;

    public static String dir = System.getProperty("user.dir");

    public static int fps;
    public static int fpsCount;
    public static int tps = 20;
    public static int tpsCount;
    public float clockSubtract = 0;

    public static long tickElapsedTime = 0;
    public static long frameElapsedTime = 0;

    public static RenderDispatcher renderSystem;

    public void startEngine() {
        try{
            init();
        } catch (WindowCreateFailure e){
            e.printStackTrace();
            return;
        }
        loop();

        // Free the window callbacks and destroy the window
        glfwFreeCallbacks(window);
        glfwDestroyWindow(window);

        // Terminate GLFW and free the error callback
        glfwTerminate();
        Objects.requireNonNull(glfwSetErrorCallback(null)).free();

        try{
            runAllAnnotatedWith(Exit.class);
        } catch (Exception e){
            e.printStackTrace();
        }
    }

    public static Set<Method> getAllAnnotatedWith(Class<? extends Annotation> annotation){
        Reflections reflections = new Reflections(new ConfigurationBuilder()
                .setUrls(ClasspathHelper.forJavaClassPath())
                .setScanners(new MethodAnnotationsScanner()));
        return reflections.getMethodsAnnotatedWith(annotation);
    }

    public static void runAllAnnotatedWith(Class<? extends Annotation> annotation)
            throws Exception {
        runMethods(getAllAnnotatedWith(annotation), null);
    }
    public static void runMethods(Set<Method> methods, Object[] parameters) throws InvocationTargetException, IllegalAccessException {
        for (Method m : methods) {
            // for simplicity, invokes methods as static without parameters
            m.invoke(m.getDeclaringClass(), parameters);
        }
    }

    public static void mac_setIcon(){
        try{
            // Read the icon file into a byte array
            byte[] iconData = Files.readAllBytes(Paths.get(dir + Main.gameFolder + "/engine/JELogo.icns"));

            // Initialize the Objective-C client
            Client client = Client.getInstance();

            // Create an NSData object with the icon data
            Proxy nsData = client.sendProxy("NSData", "dataWithBytes:length:", iconData, iconData.length);

            // Create an NSImage object with the NSData
            Object nsImage = client.sendProxy("NSImage", "alloc").send("initWithData:", nsData);

            // Set the application icon image
            client.sendProxy("NSApplication", "sharedApplication").send("setApplicationIconImage:", nsImage);
        } catch (IOException e) {
            System.out.println("Warning: Failed to set icon application!");
        }

    }

    public static void win_linux_setIcon(){
        //TODO: verify this works on linux
        ByteImage icon = TextureLoader.loadTextureToByteImage(dir + Main.gameFolder + "/img/JELogo.png");
        GLFWImage glfwImage = GLFWImage.malloc();
        GLFWImage.Buffer buffer = GLFWImage.malloc(1);
        glfwImage.set(icon.w, icon.h, icon.data);
        buffer.put(0, glfwImage);
        glfwSetWindowIcon(window, buffer);
    }

    public static void setIcon(){
        if (glfwGetPlatform() == GLFW_PLATFORM_COCOA){
            mac_setIcon();
        }else{
            win_linux_setIcon();
        }
    }

    private void init() throws WindowCreateFailure {
        System.out.println("Starting JoshEngine with LWJGL " + Version.getVersion());
        try {
            if (Files.exists(Path.of(dir + Main.gameFolder + "/"))){
                System.out.println("Engine dir exists and was found.");
            }else{
                Files.createDirectory(Path.of(dir + Main.gameFolder + "/"));
                System.out.println("Created engine dir succesfully.");
            }

        } catch (IOException e){
            e.printStackTrace();
            System.out.println("Could not find engine directory or create it! Textures will not load!");
        }
        // Set up an error callback. The default implementation
        // will print the error message in System.err.
        GLFWErrorCallback.createPrint(System.err).set();

        if ( !glfwInit() )
            throw new IllegalStateException("Unable to initialize GLFW");

        // Configure GLFW
        glfwDefaultWindowHints(); // optional, the current window hints are already the default (except ogl context stuff)
        glfwWindowHint(GLFW_VERSION_MAJOR, 1);
        glfwWindowHint(GLFW_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE); // the window will not stay hidden after creation
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // the window will be resizable
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE); //could not be me
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);

        width = 854;
        height = 480;
        currentWidth = width;
        currentHeight = height;

        // Create the window
        window = glfwCreateWindow(width, height, "JoshEngine", NULL, NULL);

        if ( window == NULL )
            throw new WindowCreateFailure();


        //keybord
        keyboard = new KeyboardHandler(window);
        //moose
        mouse = new MouseHandler(window);

        // Get the thread stack and push a new frame
        try ( MemoryStack stack = stackPush() ) {
            IntBuffer pWidth = stack.mallocInt(1); // int*
            IntBuffer pHeight = stack.mallocInt(1); // int*

            // Get the window size passed to glfwCreateWindow
            glfwGetWindowSize(window, pWidth, pHeight);

            // Get the resolution of the primary monitor
            GLFWVidMode vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

            // Center the window
            glfwSetWindowPos(
                    window,
                    (Objects.requireNonNull(vidmode).width() - pWidth.get(0)) / 2,
                    (vidmode.height() - pHeight.get(0)) / 2
            );
        } // the stack frame is popped automatically

        // Make the OpenGL context current
        glfwMakeContextCurrent(window);
        // Enable v-sync
        glfwSwapInterval(1);

        renderSystem = new RenderDispatcher();

        camera = new Camera(new Vector3f(0f, 0f, 0f), new Vector3f(0f, 0f, 0f));

        JoshShaderLoader.init();

        setIcon();

        // Make the window visible
        glfwShowWindow(window);

        // don delete shit here
        GL.createCapabilities();

        // Set the clear color
        glClearColor(0f, 0f, 0f, 0.0f);

        GLFW.glfwSetFramebufferSizeCallback(window, new GLFWFramebufferSizeCallback() {
            @Override
            public void invoke(long window, int _width, int _height) {
                int width = _width/2;
                int height = _height/2;

                if ((float)width/height>(float)Main.width/Main.height){
                    GLFW.glfwSetWindowSize(window, width, (int)(width*((float)Main.height/Main.width)));
                    currentWidth = width;
                    currentHeight = (int)(width*((float)Main.height/Main.width));
                } else if ((float)width/height<(float)Main.width/Main.height) {
                    GLFW.glfwSetWindowSize(window, (int)(height*((float)Main.width/Main.height)), height);
                    currentWidth = (int)(height*((float)Main.width/Main.height));
                    currentHeight = height;
                }
            }
        });
    }

    private void loop() {
        System.out.println("Starting game loop");
        glfwSetTime(0);

        long tickLastUpdateTime = System.currentTimeMillis();
        long frameLastUpdateTime = System.currentTimeMillis();


        try{
            runAllAnnotatedWith(Startup.class);
        } catch (Exception e){
            e.printStackTrace();
            return;
        }

        Set<Method> preRender = getAllAnnotatedWith(PreRender.class);
        Set<Method> postRender = getAllAnnotatedWith(PostRender.class);

        Set<Method> preRenderNP = getAllAnnotatedWith(PreRenderNP.class);
        Set<Method> postRenderNP = getAllAnnotatedWith(PostRenderNP.class);

        Set<Method> preTick = getAllAnnotatedWith(PreTick.class);
        Set<Method> postTick = getAllAnnotatedWith(PostTick.class);
        while (!glfwWindowShouldClose(window) ) {
            if (glfwGetTime()-clockSubtract > 1f) {
                fps = fpsCount;
                fpsCount = 0;

                tps = tpsCount;
                tpsCount = 0;

                clockSubtract += 1f;
                System.out.println("FPS:" + fps + " TPS: " + tps);
            }

            long now = System.currentTimeMillis();
            tickElapsedTime = now - tickLastUpdateTime;
            frameElapsedTime = now - frameLastUpdateTime;

            if (frameElapsedTime  >= frameWait){
                deltaTime = frameElapsedTime/1000f;
                keyboard.update();
                mouse.update();

                try{
                    runMethods(preRenderNP, null);
                    if (runEvents) runMethods(preRender, null);
                } catch (Exception e){
                    e.printStackTrace();
                    return;
                }
                GL13.glLightModelfv(GL13.GL_LIGHT_MODEL_AMBIENT, ambient);
                renderSystem.render(window);
                try{
                    runMethods(postRenderNP, null);
                    if (runEvents) runMethods(postRender, null);
                } catch (Exception e){
                    e.printStackTrace();
                    return;
                }
                fpsCount++; //Rendered a frame so yeah
                frameLastUpdateTime = now;
            }

            if (tickElapsedTime >= tickWait) {
                tickDeltaTime = (Main.tickElapsedTime/1000f);
                if (runEvents) {
                    try {
                        runMethods(preTick, null);
                    } catch (Exception e) {
                        e.printStackTrace();
                        return;
                    }
                    for (GameObject gameObject : gameObjects) {
                        gameObject.setLastTransform(gameObject.getTransform());
                        gameObject.getComponents().forEach(Component::onTick);
                    }
                    try {
                        runMethods(postTick, null);
                    } catch (Exception e) {
                        e.printStackTrace();
                        return;
                    }
                }
                tpsCount++;
                tickLastUpdateTime = now;
            }
        }
    }

    public void reset(){
        runEvents = false;
        gameObjects.clear();
        fps = (int) targetFps;
        tps = (int) targetTps;
        camera.transform = new Transform(new Vector3f());
        camera.lastTransform = new Transform(new Vector3f());
    }
}
