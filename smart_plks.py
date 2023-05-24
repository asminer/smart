import random

N = 3
num_samples = 2
battery = 7
forbidden_s= 2*N//3

forbid_coords=[]
for i in range(0,forbidden_s):
  x = random.randint(0,N-1)
  y = random.randint(0,N-1)
  while (x,y) in forbid_coords:
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
  while (x,y) in (sample_coords or forbid_coords):
    x = random.randint(0,N-1)
    y = random.randint(0,N-1)
  sample_coords.append((x,y))

output = """
#GraphDisplayStyle OUTGOING
#StatesetPrintIndexes false

pn plks := {
  place """

places = []
for i in range(0,N):
  for j in range(0,N):
    places.append(f"p{i}_{j}")

for (x,y) in sample_coords:
  places.append(f"sample_{x}_{y}")

places.append("samples")
places.append("battery")

output += ",".join(places) + ";"

transitions = []
for i in range(0,N):
  for j in range(0,N):
    if i-1 >= 0:
      transitions.append(f"t{i}_{j}__{i-1}_{j}")
    if i+1 <= N-1:
      transitions.append(f"t{i}_{j}__{i+1}_{j}")
    if j-1 >= 0:
      transitions.append(f"t{i}_{j}__{i}_{j-1}")
    if j+1 <= N-1:
      transitions.append(f"t{i}_{j}__{i}_{j+1}")

for (x,y) in sample_coords:
  transitions.append(f"sample_{x}_{y}_take")

output += "\n  trans " + ",".join(transitions) + ";"

output += "\n  init("

inits = []
inits.append("p0_0:1")
inits.append(f"battery:{battery}")
for (x,y) in sample_coords:
  inits.append(f"sample_{x}_{y}:1")

output += ",".join(inits) + ");"

arcs = []
for i in range(0,N):
  for j in range(0,N):
    if i-1 >= 0:
      arcs.append(f"p{i}_{j} : t{i}_{j}__{i-1}_{j}")
      arcs.append(f"t{i}_{j}__{i-1}_{j} : p{i-1}_{j}")
      if (i,j) not in forbid_coords:
        arcs.append(f"battery : t{i}_{j}__{i-1}_{j}")
    if i+1 <= N-1:
      arcs.append(f"p{i}_{j} : t{i}_{j}__{i+1}_{j}")
      arcs.append(f"t{i}_{j}__{i+1}_{j} : p{i+1}_{j}")
      if (i,j) not in forbid_coords:
        arcs.append(f"battery : t{i}_{j}__{i+1}_{j}")
    if j-1 >= 0:
      arcs.append(f"p{i}_{j} : t{i}_{j}__{i}_{j-1}")
      arcs.append(f"t{i}_{j}__{i}_{j-1} : p{i}_{j-1}")
      if (i,j) not in forbid_coords:
        arcs.append(f"battery : t{i}_{j}__{i}_{j-1}")
    if j+1 <= N-1:
      arcs.append(f"p{i}_{j} : t{i}_{j}__{i}_{j+1}")
      arcs.append(f"t{i}_{j}__{i}_{j+1} : p{i}_{j+1}")
      if (i,j) not in forbid_coords:
        arcs.append(f"battery : t{i}_{j}__{i}_{j+1}")

for (x,y) in sample_coords:
  arcs.append(f"sample_{x}_{y} : sample_{x}_{y}_take")
  arcs.append(f"sample_{x}_{y}_take : samples")

output += "\n  arcs(" + ",".join(arcs) + ");"

guards = []
for (x,y) in sample_coords:
  guards.append(f"sample_{x}_{y}_take : tk(p{x}_{y}) > 0")

output += "\n  guard(" + ",".join(guards) + ");"

decisions = []
cost =[]
for i in range(1,num_samples+1):
  decisions.append(f"s_min{i}")
  cost.append(f"s_min{i}:{(num_samples-i+1)**2}")

for i in range(1,4):
  decisions.append(f"battery_min{i}")
  cost.append(f"battery_min{i}:{(4-i+1)**2}")

output += "\n  decision " + ",".join(decisions) + ";\n"
output+= "cost("+ ",".join(cost)+");\n"


dec_enables = []
for i in range(1,num_samples+1):
  s_min_list=[]
  for j  in range(1,num_samples+1):
    if j!=i:
      s_min_list.append(f"!is_taken(s_min{j})")
  dec_enables.append(f"s_min{i}:"+"&".join(s_min_list))

for i in range(1,4):
  b_min_list=[]
  for j  in range(1,4):
    if j!=i:
      b_min_list.append(f"!is_taken(battery_min{j})")
  dec_enables.append(f"battery_min{i}:"+"&".join(b_min_list))
  
output+="enable_decision("+ ",".join(dec_enables)+");"
output += f"""
  bigint n_states := num_states;
  bigint n_arcs := num_arcs;
  stateset r := reachable;
  stateset prop := (potential(tk(p0_0)==1)) -> EU(reachable, potential(tk(p{N-1}_{N-1})==1) & ("""

battery_p=[]
for i in range(1,4):
  battery_p.append(f"(potential(dec_value(battery_min{i}) & tk(battery)> {int(battery*(i/4))}))")
output+= "|".join(battery_p)+ ") & ("

props = []
for i in range(1,num_samples+1):
  props.append(f"(potential(dec_value(s_min{i}) & tk(samples)>{i-1}))")

output += "|".join(props) + "));"

output += """
  bool test := min_decision_cost(prop);
};

print(plks.n_states,"\\n");
print(plks.n_arcs,"\\n");
print(plks.r,"\\n");
print("--------\\n");
print(plks.test,"\\n");"""

print(output)
