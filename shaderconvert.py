import os
import subprocess

try:
    naga_version_output = subprocess.run(['naga', '--version'], stdout=subprocess.PIPE).stdout.decode('utf-8')
except:
    print("naga --version did not execute successfully. Do you have it installed correctly (cargo install naga-cli)?")
    exit(1)
if int(naga_version_output[:naga_version_output.find(".")]) < 23:
    print("Naga must be installed with at least version 23 for full support!")

os.system("") # Why does this fix color escapes????
os.chdir("engineRuntime/shaders")
files = os.listdir()
glsl_shaders = []

for file in files:
    if file.endswith(".glsl"):
        glsl_shaders.append(file)

print("Found", len(glsl_shaders), "GLSL shaders to convert to WGSL")

if not os.path.exists("./glsl_to_wgsl/"):
    os.mkdir("./glsl_to_wgsl/")

if not os.path.exists("./wgsl_conv/"):
    os.mkdir("./wgsl_conv/")

commands = []

for shader in glsl_shaders:
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
    code_lines = code.splitlines()
    translate_header = code_lines.pop(0).split(" ")
    simplify_inverse_transpose = False
    for item in translate_header:
        if item.find("simplify_inv_transp") != -1:
            simplify_inverse_transpose = True
    code = ""
    combined_sampler_dimension_tracker = {}
    push_constant_append = False
    print()
    for line in code_lines:
        # Generally, any sampler2D in JoshEngine should be a combined image sampler.
        # However, I just want to account for anyone doing things weirdly,
        # even though this should be impossible with our setup.
        if line.find("sampler") != -1 and line.find("uniform") != -1:
            set_index = line.find(",")
            equals_index = line.find("=") + 1
            set_number = int(line[equals_index:set_index])

            line = line.replace("sampler", "texture")
            last_space = line.rfind(" ") + 1
            print("- Replaced " + line[last_space:-1] + "'s layout qualifier and uniforms")
            combined_sampler_dimension_tracker[line[last_space:-1]] = line[line.find("texture") + 7:last_space - 1]
            line = line[:last_space] + "_je_texture_" + line[last_space:] + "\nlayout(set = " + str(set_number + 1) + ", binding = 1) uniform sampler _je_sampler_" + line[last_space:]
        elif line.find("texture(") != -1:
            statement_begin = line.find("texture(")
            statement_end = line[statement_begin:].find(",")
            combined_sampler_name = line[statement_begin + 8:statement_end + statement_begin]
            dimension = combined_sampler_dimension_tracker[combined_sampler_name]
            line = line.replace(combined_sampler_name, "sampler" + dimension + "(_je_texture_" + combined_sampler_name + ", _je_sampler_" + combined_sampler_name + ")")
            print("- Replaced texture value evaluation of " + combined_sampler_name)
        if line.find("set") != -1:
            set_index = line.find(",")
            equals_index = line.find("=") + 1
            set_number = int(line[equals_index:set_index])
            line = "layout(set = " + str(set_number + 1) + line[set_index:]
            print ("- Incremented set", set_number, "to make space for constants")
        if line.find("push_constant") != -1:
            line = line.replace("layout(push_constant)", "layout(set = 0, binding = 0)")
            push_constant_append = simplify_inverse_transpose
            print("- Changed push constant to uniform(0)")
        if line.find("};") != -1 and push_constant_append:
            push_constant_append = False
            line = "mat4 _je_normalMatrix;\n" + line
            print("- Added _je_normalMatrix to push constants (simplify_inv_trans)")
        if line.find("transpose(inverse(") != -1 and simplify_inverse_transpose:
            transpose_inverse_location = line.find("transpose(inverse(")
            line = line.replace(line[transpose_inverse_location:transpose_inverse_location + line[transpose_inverse_location:].find(")") + 2], "_je_normalMatrix")
            print("- Simplified inverse transpose statement to _je_normalMatrix")
        code += line + "\n"
    print()

    tempf = open("./glsl_to_wgsl/"+shader, "w")
    tempf.write(code)
    tempf.close()

for command in commands:
    print(command)
    subprocess.run(command.split(" "))