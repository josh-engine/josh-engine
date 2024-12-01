import os
import sys
import subprocess

try:
    nagaVersionOutput = subprocess.run(['naga', '--version'], stdout=subprocess.PIPE).stdout.decode('utf-8')
except:
    print("naga --version did not execute successfully. Do you have it installed correctly (cargo install naga-cli)?")
    exit(1)
if int(nagaVersionOutput[:nagaVersionOutput.find(".")]) < 23:
    print("Naga must be installed with at least version 23 for full support!")

os.system("") # Why does this fix color escapes????
os.chdir("engineRuntime/shaders")
files = os.listdir()
glslShaders = []

for file in files:
    if file.endswith(".glsl"):
        glslShaders.append(file)

print("Found", len(glslShaders), "GLSL shaders to convert to WGSL")

if not os.path.exists("./glsl_to_wgsl/"):
    os.mkdir("./glsl_to_wgsl/")

if not os.path.exists("./wgsl_conv/"):
    os.mkdir("./wgsl_conv/")

commands = []

for shader in glslShaders:
    file = open(shader, 'r')
    code = file.read()
    file.close()

    command = "naga --input-kind glsl "
    if code.startswith("// JE_TRANSLATE vertex"):
        command = command + "--shader-stage vert "
    elif code.startswith("// JE_TRANSLATE fragment"):
        command = command + "--shader-stage frag "
    else:
        print(shader, "was not marked with JE_TRANSLATE.")
        print("For conversion support, please specify with\n// JE_TRANSLATE vertex\nor\n// JE_TRANSLATE fragment\naccordingly, before the #version 420.")
        print() # newline
        continue

    command = command + "./glsl_to_wgsl/" + shader + " ./wgsl_conv/" + shader[:-5] + ".wgsl"

    commands.append(command)

    code = code.replace("#version 420", "#version 440")
    print(shader)
    codeLines = code.splitlines()
    code = ""
    combinedSamplerDimensionTracker = {}
    for line in codeLines:
        # Generally, any sampler2D in JoshEngine should be a combined image sampler.
        # However, I just want to account for anyone doing things weirdly,
        # even though this should be impossible with our setup.
        if line.find("sampler") != -1 and line.find("uniform") != -1:
            setIndex = line.find(",")
            equalsIndex = line.find("=") + 1
            setNumber = int(line[equalsIndex:setIndex])

            line = line.replace("sampler", "texture")
            lastSpace = line.rfind(" ")+1
            print("- Replaced " + line[lastSpace:-1] + "'s layout qualifier and uniforms")
            combinedSamplerDimensionTracker[line[lastSpace:-1]] = line[line.find("texture")+7:lastSpace-1]
            line = line[:lastSpace]+"_je_texture_"+line[lastSpace:] + "\nlayout(set = " + str(setNumber+1) + ", binding = 1) uniform sampler _je_sampler_" + line[lastSpace:]
        elif line.find("texture(") != -1:
            statementBegin = line.find("texture(")
            statementEnd = line[statementBegin:].find(",")
            combinedSamplerName = line[statementBegin+8:statementEnd+statementBegin]
            dimension = combinedSamplerDimensionTracker[combinedSamplerName]
            line = line.replace(combinedSamplerName, "sampler"+dimension+"(_je_texture_"+combinedSamplerName+", _je_sampler_"+combinedSamplerName+")")
            print("- Replaced texture value evaluation of " + combinedSamplerName)
        if line.find("set") != -1:
            setIndex = line.find(",")
            equalsIndex = line.find("=") + 1
            setNumber = int(line[equalsIndex:setIndex])
            line = "layout(set = "+str(setNumber+1)+line[setIndex:]
            print ("- Incremented set", setNumber, "to make space for constants")
        if line.find("push_constant") != -1:
            line = line.replace("layout(push_constant)", "layout(set = 0, binding = 0)")
            print("- Changed push constant to uniform(0)")
        code += line + "\n"
    print()

    tempf = open("./glsl_to_wgsl/"+shader, "w")
    tempf.write(code)
    tempf.close()

for command in commands:
    print(command)
    subprocess.run(command.split(" "))