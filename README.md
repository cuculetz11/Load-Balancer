# Load-Balancer
    Tema 2 - Distributed Database

  ## -LRU_CACHE-
  Am implementat memoria cache folosind un hashtable si o lista dubla
  inlantuita. In hashtable retin ca si key pointer la numele documentului 
  din baza de date, iar ca valoare pointer la nodul din lista dublu 
  inlantuita. Aceasta lista are ca data pointer la structura info ce are key
  ca numele documentului si valoare ca continutul documentului(am facut acest
  lucru ca sa mi fie usor sa suprasciu atunci cand documentul exista deja in
  cache).
      Ma folosesc de aceasta lista dublu inaltuita ca sa am memoria in 
  ordinea utilizarrii, In hashtable retin nodul din lista pentru ca sa
  pot face eliminarea si adaugarea in O(1). Am vazut cazul in care am
  documente in cache, iar cand trebuie sa dau get la continut sau sa-l
  modific trebuie ca in lista dublu inlantuita sa pun ca cel mai recent
  folosit acest document, folosind alta implmentare m-as fi plimbat prin
  lista pana gaseam acel document,il eliminam si apoi il puneam la inceput
  astfel, nu mai era O(1);
  -in rest sper ca numele variabilelor si ale functiiolr sunt destul de
  sugestive pentru intelegerea implementarii

  ## -SERVER-
  Am implementat serverul folosind un hashtable ca baza de date, o coada
  de task-uri si o memorie cache ca cea prezentata mai sus. Hashtable-ul
  este redimensionat la fiecare 2 * numarul de elemente din hashtable, 
  nestiind numarul de documente maxim din baza de date, am ales aceasta
  implementare deoarece este foare eficienta ca si timp. In functie de
  ce request primesc efectuez: 
      -EDIT - bag in coada de task-uri un task de editare a documentului
      iar cand ajung la acest task, verific daca documentul este in cache
      daca este il modific si il pun ca cel mai recent folosit, daca nu
      este verific daca exista in baza de date, daca exista il suprascriu 
      direct acolo si apoi il pun in cache, daca nu exista in baza de date
      il adaug in baza de date si il pun in cache
      -GET - prima data efectuez toate task-urile din coada, apoi am 3 
  cazuri: 
          -daca documentul este in cache, il returnez
          -daca documentul nu este in cache, il pun in cache si il returnez
          -daca documentul nu este in cache si nu exista in baza de date

  ## -LOAD_BALANCER-
  Am implementat load balancer folosind struct de server(un vector de
  servere), numarul de servere. Am inteles ideea din spate numai ca 
  nu am reusit sa o implementez corect. Pentru fiecare request,
  aleg serverul cel mai mic ce are hash-ul id-ului mai mare decat hash-ul
  numelui documentului. Daca nu exista niciun server cu hash-ul mai mare
  decat hash-ul numelui documentului, aleg serverul cu hash-ul cel mai
  mic. Apoi cand trebuie sa adaug un server, creez o structura hash_ring_t
  ce contine pointer la un server si hash-ul lui. Adaug aceasta structura
  intr-un vector de structuri hash_ring_t si sortez vectorul in functie de
  hash-ul fiecarui server. Apoi gasesc serverul destinatie si serverul
  sursa, apoi caut toate documenetele din sursa ce au hash-ul mai mic
  decat hash-ul serverului destinatie si le mut in serverul destinatie.
  Cand gasesc serverul destinatie, daca exista vreun task in coada de task-uri
  le fac.
      Pentru functia de remove iau serverul sursa ii fac toate requesturile
  din coada, apoi caut serverul ce este urmatorul pe inel si-i transmit toate
  documentele din sursa, apoi eliberez serverul sursa.
          
