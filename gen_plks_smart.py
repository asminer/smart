import random

N = 3
num_samples = 2
battery = 7

sample_coords = []
for i in range(0,num_samples):
  x = random.randint(0,N-1)
  y = random.randint(0,N-1)
  while (x,y) in sample_coords:
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
      arcs.append(f"battery : t{i}_{j}__{i-1}_{j}")
    if i+1 <= N-1:
      arcs.append(f"p{i}_{j} : t{i}_{j}__{i+1}_{j}")
      arcs.append(f"t{i}_{j}__{i+1}_{j} : p{i+1}_{j}")
      arcs.append(f"battery : t{i}_{j}__{i+1}_{j}")
    if j-1 >= 0:
      arcs.append(f"p{i}_{j} : t{i}_{j}__{i}_{j-1}")
      arcs.append(f"t{i}_{j}__{i}_{j-1} : p{i}_{j-1}")
      arcs.append(f"battery : t{i}_{j}__{i}_{j-1}")
    if j+1 <= N-1:
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

decisions = []
for i in range(1,num_samples+1):
  decisions.append(f"s_min{i}")

output += "\n  decision " + ",".join(decisions) + ";"

# dec_enables = []
# for i in range(0,num_samples):
#   for j  in range(0,i):
#     dec_enables.append(f"")

output += f"""
  bigint n_states := num_states;
  bigint n_arcs := num_arcs;
  stateset r := reachable;
  stateset prop := (potential(tk(p0_0)==1)) -> EU(reachable, potential(tk(p{N-1}_{N-1})==1) & (potential(tk(battery) > 0)) & ("""

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