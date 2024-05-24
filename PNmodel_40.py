import random

#forbid_coords=[(0, 12), (0, 19), (1, 6), (1, 7), (1, 12), (2, 18), (3, 8), (3, 9), (3, 11), (3, 28), (4, 2), (4, 4), (6, 3), (6, 4), (6, 6), (6, 18), (6, 23), (7, 9), (7, 12), (7, 23), (8, 19), (9, 9), (9, 13), (10, 10), (10, 15), (10, 39), (13, 13), (14, 1), (14, 22), (14, 31), (15, 0), (16, 25), (17, 17), (20, 21), (22, 37), (24, 2), (27, 18), (27, 28), (28, 10), (31, 5)]
#sample_coords=[(1, 0), (1, 13), (1, 17), (1, 22), (1, 23), (2, 9), (4, 26), (9, 3), (9, 17), (11, 0), (11, 11), (14, 8), (15, 4), (19, 10), (19, 18), (19, 22), (20, 28), (29, 1), (30, 6), (37, 8)]
forbid_coords= [(0, 19), (1, 6), (1, 7), (2, 18), (3, 8), (3, 9), (3, 11), (4, 2), (4, 4), (6, 3), (6, 4), (6, 6), (6, 18), (7, 9), (9, 9), (9, 13), (10, 15), (13, 13), (14, 1), (15, 0)]
sample_coords=[(1, 0), (1, 13), (1, 17), (1, 22), (4, 26), (9, 3), (9, 17), (11, 0), (14, 8), (15, 4), (19, 10), (19, 18), (19, 22), (20, 28), (29, 1)]
def generate_smart(N: int, num_samples: int, B: int, forbidden_s: int) -> str:

  # forbid_coords=[]
  # for i in range(0,forbidden_s-len(forbid_coords)):
  #   x = random.randint(0,N-1)
  #   y = random.randint(0,N-1)
  #   while (x,y) in forbid_coords or (x,y) == (N-1,N-1) or (x,y) == (0,0):
  #     x = random.randint(0,N-1)
  #     y = random.randint(0,N-1)
  #   forbid_coords.append((x,y))

 
  # sample_coords = []
  # for i in range(0,num_samples-len(sample_coords)):
  #   x = random.randint(0,N-1)
  #   y = random.randint(0,N-1)
  #   while (x,y) in sample_coords or (x,y) in forbid_coords:
  #     x = random.randint(0,N-1)
  #     y = random.randint(0,N-1)
  #   sample_coords.append((x,y))

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
      cost.append(f"battery_min{i}:{int(8*25)}")
    if(i==2):
      cost.append(f"battery_min{i}:{int(6*25)}")
    if(i==3):
      cost.append(f"battery_min{i}:{int(4*25)}")
    if(i==4):
      cost.append(f"battery_min{i}:{int(2*25)}")
    if(i==5):
      cost.append(f"battery_min{i}:{int(1*25)}")

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
    bigint n_arcs := num_arcs;
    stateset prop := (potential(tk(p0_0)==1)) -> EU(reachable, potential( (tk(p{N-1}_{N-1})==1) & ( """

  battery_p=[]
  for i in range(1,6):
    if (i==1):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 0)")
    else:
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > {((2*i) -1)})")
    '''
    if (i==2):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 1)")
    if (i==3):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 3)")
    if (i==4):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 5)")
    if (i==5):
      battery_p.append(f"(dec_value(battery_min{i}) & tk(battery) > 7)")
    '''
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
  print(plks.n_arcs,"\\n");
  print("Model generation: ", stop_timer(0), " seconds\\n");
  print(plks.test,"\\n");
  """

  return output

smart_src = generate_smart(25, 15, 40, 20)
testname = "final_plks_"+"_".join([str(25), str(15), str(40), str(20)])
    
    #print(smart_src)
  # print(sample_coords)
  # print(forbid_coords)
with open (testname+".sm","w") as file1:
    file1.write(smart_src)