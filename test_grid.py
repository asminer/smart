import random
sample=[]
fcell=[]
def test_rand(s,f,n):
  for i in range(0,s-len(sample)):
    x= random.randint(0,n-1)
    y= random.randint(0,n-1)
    while (x,y) in fcell or (x,y)==(n-1,n-1) or (x,y)==(0,0):
      x= random.randint(0,n-1)
      y=random.randint(0,n-1)
    sample.append((x,y))
  for i in range(0,f-len(fcell)):
    x=random.randint(0,n-1)
    y= random.randint(0,n-1)
    while (x,y) in fcell or (x,y)==(0,0) or (x,y)==(n-1,n-1):
      x= random.randint(0,n-1)
      y=random.randint(0,n-1)
    fcell.append((x,y)) 
print("forbidden",fcell)
print("sample",sample)
test_rand(3,2,3)
print("forbidden", fcell)
print("sample",sample)
test_rand(4,3,4)
print("forbidden",fcell)
print("sample",sample)

'''
def generate_smart(N: int, num_samples: int, B: int, forbidden_s: int, EX: int, EX_f:int, EX_s: int) -> str:
  extra_forbidden_cell=[]
  for i in range(0,Ex_f):
    x= random.randint(0,N+EX-1)
    y= random.randint(0,N+EX-1)
    while (x,y) in forbid_coords or (x,y) == (N-1,N-1) or (x,y) == (0,0):
        x = random.randint(0,N-1)
        y = random.randint(0,N-1)
    extra_forbidden_cell((x,y))


  forbid_coords=[]
  for i in range(0,forbidden_s):
    x = random.randint(0,N-1)
    y = random.randint(0,N-1)
    while (x,y) in forbid_coords or (x,y) == (N-1,N-1) or (x,y) == (0,0):
      x = random.randint(0,N-1)
      y = random.randint(0,N-1)
    forbid_coords.append((x,y))

  extra_sample_cell=[]
  for i in range(0,EX_s):
    x = random.randint(0,N+EX-1)
    y = random.randint(0,N+EX-1)
    while (x,y) in sample_coords or (x,y) in forbid_coords:
      x = random.randint(0,N+EX-1)
      y = random.randint(0,N+EX-1)
    extra_sample_cell.append((x,y))
 
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
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 0)")
    else:
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > {((2*i) -1)})")
    # print(B*(i/4))
  output+= "|".join(battery_p)+ ") & ("

  props = []
  for i in range(1,num_samples+1):
    props.append(f"(dec_value(s_min{i}) & tk(samples)>{i-1})")

  output += "|".join(props) + ")));"

  output += f"""
    int test := min_decision_cost(prop);
  }};

  start_timer(0);
  print(plks.n_states,"\\n");
  print("Model generation: ", stop_timer(0), " seconds\\n");
  print(plks.test,"\\n");
  """

  return output

'''