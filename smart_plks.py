#!/usr/bin/python3

#from multiprocessing import Process
import random
import subprocess

def generate_smart(N: int, num_samples: int, B: int, forbidden_s: int) -> str:

  forbid_coords=[]
  for i in range(0,forbidden_s):
    x = random.randint(0,N-1)
    y = random.randint(0,N-1)
    while (x,y) in forbid_coords or (x,y) == (N-1,N-1) or (x,y) == (0,0):
      x = random.randint(0,N-1)
      y = random.randint(0,N-1)
    forbid_coords.append((x,y))

  #### no transition from battery to forbidden coords
  #### sample coords and forbidden coords need to be mutually exclusive
  #### battery min as a decision and add cost
  #### sample min and cost [need clarification]
  sample_coords = []
  for i in range(0,num_samples):
    x = random.randint(0,N-1)
    y = random.randint(0,N-1)
    while (x,y) in sample_coords or (x,y) in forbid_coords:
      x = random.randint(0,N-1)
      y = random.randint(0,N-1)
    sample_coords.append((x,y))

  output = "//\n"
  output += "// Grid " + str(N) + " x " + str(N) + " \n"
  output += "//\n"
  output += "// "
  for i in range(0,N):
    for j in range(0,N):
        if (i,j) == (0,0):
            output += "A"
            continue
        if (i,j) == (N-1,N-1):
            output += "B"
            continue
        if (i,j) in forbid_coords:
            output += "X"
            continue
        if (i,j) in sample_coords:
            output += "s"
            continue
        output += "."
    output += "\n// "
  output += "\n"

  output += """
  #GraphDisplayStyle OUTGOING
  #StatesetPrintIndexes false

  pn plks := {
    place """

  places = []
  for i in range(0,N):
    for j in range(0,N):
      if not (i,j) in forbid_coords:
        places.append(f"p{i}_{j}")

  for (x,y) in sample_coords:
    places.append(f"sample_{x}_{y}")

  places.append("samples")
  places.append("battery")

  output += ",".join(places) + ";"

  transitions = []
  for i in range(0,N):
    for j in range(0,N):
      if not (i,j) in forbid_coords:
        if i-1 >= 0 and not (i-1,j) in forbid_coords:
          transitions.append(f"t{i}_{j}__{i-1}_{j}")
        if i+1 <= N-1 and not (i+1,j) in forbid_coords:
          transitions.append(f"t{i}_{j}__{i+1}_{j}")
        if j-1 >= 0 and not (i,j-1) in forbid_coords:
          transitions.append(f"t{i}_{j}__{i}_{j-1}")
        if j+1 <= N-1 and not (i,j+1) in forbid_coords:
          transitions.append(f"t{i}_{j}__{i}_{j+1}")

  for (x,y) in sample_coords:
    transitions.append(f"sample_{x}_{y}_take")

  output += "\n  trans " + ",".join(transitions) + ";"

  output += "\n  init("

  inits = []
  inits.append("p0_0:1")
  inits.append(f"battery:{B}")
  for (x,y) in sample_coords:
    inits.append(f"sample_{x}_{y}:1")

  output += ",".join(inits) + ");"

  arcs = []
  for i in range(0,N):
    for j in range(0,N):
      if not (i,j) in forbid_coords:
        if i-1 >= 0 and not (i-1,j) in forbid_coords:
          arcs.append(f"p{i}_{j} : t{i}_{j}__{i-1}_{j}")
          arcs.append(f"t{i}_{j}__{i-1}_{j} : p{i-1}_{j}")
          arcs.append(f"battery : t{i}_{j}__{i-1}_{j}")
        if i+1 <= N-1 and not (i+1,j) in forbid_coords:
          arcs.append(f"p{i}_{j} : t{i}_{j}__{i+1}_{j}")
          arcs.append(f"t{i}_{j}__{i+1}_{j} : p{i+1}_{j}")
          arcs.append(f"battery : t{i}_{j}__{i+1}_{j}")
        if j-1 >= 0 and not (i,j-1) in forbid_coords:
          arcs.append(f"p{i}_{j} : t{i}_{j}__{i}_{j-1}")
          arcs.append(f"t{i}_{j}__{i}_{j-1} : p{i}_{j-1}")
          arcs.append(f"battery : t{i}_{j}__{i}_{j-1}")
        if j+1 <= N-1 and not (i,j+1) in forbid_coords:
          arcs.append(f"p{i}_{j} : t{i}_{j}__{i}_{j+1}")
          arcs.append(f"t{i}_{j}__{i}_{j+1} : p{i}_{j+1}")
          arcs.append(f"battery : t{i}_{j}__{i}_{j+1}")

  for (x,y) in sample_coords:
    arcs.append(f"sample_{x}_{y} : sample_{x}_{y}_take")
    arcs.append(f"sample_{x}_{y}_take : samples")

  output += "\n  arcs(" + ",".join(arcs) + ");"

  guards = []
  for (x,y) in sample_coords:
    guards.append(f"sample_{x}_{y}_take : tk(p{x}_{y}) > 0")

  output += "\n  guard(" + ",".join(guards) + ");"

  inhibits = []
  for (x,y) in sample_coords:
    if x-1 >= 0 and not (x-1,y) in forbid_coords:
      inhibits.append(f"sample_{x}_{y} : t{x}_{y}__{x-1}_{y}")
    if x+1 <= N-1 and not (x+1,y) in forbid_coords:
      inhibits.append(f"sample_{x}_{y} : t{x}_{y}__{x+1}_{y}")
    if y-1 >= 0 and not (x,y-1) in forbid_coords:
      inhibits.append(f"sample_{x}_{y} : t{x}_{y}__{x}_{y-1}")
    if y+1 <= N-1 and not (x,y+1) in forbid_coords:
      inhibits.append(f"sample_{x}_{y} : t{x}_{y}__{x}_{y+1}")

  output += "\n  inhibit(" + ",".join(inhibits) + ");"

  decisions = []
  cost =[]
  for i in range(1,num_samples+1):
    decisions.append(f"s_min{i}")
    cost_t=0
    for j in range(1,i+1):
      cost_t+= 1/j
    cost.append(f"s_min{i}:{int((num_samples-cost_t)*100)}")

  for i in range(1,6):
    decisions.append(f"battery_min{i}")
    if(i==1):
      cost.append(f"battery_min{i}:{int(8*100)}")
    if(i==2):
      cost.append(f"battery_min{i}:{int(6*100)}")
    if(i==3):
      cost.append(f"battery_min{i}:{int(4*100)}")
    if(i==4):
      cost.append(f"battery_min{i}:{int(2*100)}")
    if(i==5):
      cost.append(f"battery_min{i}:{int(1*100)}")

  output += "\n  decision " + ",".join(decisions) + ";\n"
  output+= "cost("+ ",".join(cost)+");\n"


  dec_enables = []
  for i in range(1,num_samples+1):
    s_min_list=[]
    for j  in range(1,num_samples+1):
      if j!=i:
        s_min_list.append(f"!is_taken(s_min{j})")
    dec_enables.append(f"s_min{i}:"+"&".join(s_min_list))

  for i in range(1,6):
    b_min_list=[]
    for j  in range(1,6):
      if j!=i:
        b_min_list.append(f"!is_taken(battery_min{j})")
    dec_enables.append(f"battery_min{i}:"+"&".join(b_min_list))
    
  output+="enable_decision("+ ",".join(dec_enables)+");"
  output += f"""
    bigint n_states := num_states;
    stateset prop := (potential(tk(p0_0)==1)) -> EU(reachable, potential( (tk(p{N-1}_{N-1})==1) & ( """

  battery_p=[]
  for i in range(1,6):
    if (i==1):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 0))")
    if (i==2):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 1))")
    if (i==3):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 3))")
    if (i==4):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 5))")
    if (i==5):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 7))")
    # print(B*(i/4))
  output+= "|".join(battery_p)+ ") & ("

  props = []
  for i in range(1,num_samples+1):
    props.append(f"((dec_value(s_min{i}) & tk(samples)>{i-1}))")

  output += "|".join(props) + "));"

  output += f"""
    int test := min_decision_cost(prop);
  }};

  start_timer(0);
  print(plks.n_states,"\\n");
  print("Model generation: ", stop_timer(0), " seconds\\n");
  print(plks.test,"\\n");
  """

  return output
dims = [15]
num_samples = [3]
max_battery = 30
forbidden_s = [40]

num_tests = [1]

for i in range(0,len(dims)):
  for j in range(0,num_tests[i]):
    smart_src = generate_smart(dims[i], num_samples[i], max_battery, forbidden_s[i])
    testname = "range_plks_"+"_".join([str(dims[i]), str(num_samples[i]), str(max_battery), str(forbidden_s[i]), str(j)])
    with open(testname+".sm","w") as f:
      f.write(smart_src)
    result = subprocess.run(["./bin-devel/bin/smart",testname+".sm"], capture_output=True, text=True)
    with open(testname+".out","w") as f:
      f.write(result.stdout)
    print(f"done with {testname}")


'''
def run_test(i, dim, num_samples, forbidden_s):
  for j in range(0,i):
    smart_src = generate_smart(dim, num_samples, forbidden_s)
    testname = "plks_"+"_".join([str(dim), str(num_samples), str(forbidden_s), str(j)])
    with open(testname+".sm","w") as f:
        f.write(smart_src)
    result = subprocess.run(["./bin-release/bin/smart",testname+".sm"], stdout=subprocess.PIPE,timeout=3600)
    with open(testname+".out","w") as f:
        f.write(result.stdout.decode('utf-8'))
    print(f"done with {testname}")  


# dims = [5,10,15,20,25,30,35]
dims = [5,10,15]
num_samples = [n//2 for n in dims]
forbidden_s = [2*n//3 for n in dims]

num_tests = [6,6,6]

processes = []

for i in range(0,len(dims)):
  processes.append(Process(target=run_test, args=(num_tests[i], dims[i], num_samples[i], forbidden_s[i])))

for p in processes:
    p.start()

for p in processes:
    p.join()
    print("done with "+str(p))
'''
