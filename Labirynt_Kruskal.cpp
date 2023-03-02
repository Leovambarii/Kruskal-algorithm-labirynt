//  Jakub Ostrowski

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

#define delay_time_ 500  // Odstęp czasowy pomiędzy kolejnymi połączeniami w milisekundach (Funkcjonalna jedynie gdy delay_ = true)

template <typename T>
class Iterator {
    public:
        virtual ~Iterator (){;}
        Iterator(){;}
        virtual bool IsDone() = 0;
        virtual T & operator*() = 0;
        virtual void operator++() = 0;
};

template <typename T>
class Visitor {
    public:
        virtual void Visit (T& element) = 0;
        virtual bool IsDone() { return false; }
};

class Vertex {
    int number;
    public:
        int weight;
        std::string label;
        Vertex(int n) { number = n; }
        int Number() const { return number; }
};

class Edge {
    protected:
        Vertex* v0;
        Vertex* v1;
    public:
        int weight;
        std::string label;
        Edge (Vertex *V0, Vertex* V1) {
            v0 = V0;
            v1 = V1;
        }
        Vertex* V0 (){ return v0; };
        Vertex* V1 (){ return v1; };
        Vertex* Mate (Vertex *v) {
            if(v->Number() == v0->Number())
                return v1;
            return v0;
        }
};

class CountingVisitor : Visitor<Vertex>{
    public:
        int visited_Vertices;
        CountingVisitor(): visited_Vertices(0) {}
        void Visit(Vertex& element) { visited_Vertices++; }
        void Zeruj() { visited_Vertices = 0; }
        bool IsDone() { return false; }
};

class GraphAsMatrix {
    std::vector<Vertex*> vertices;
    std::vector<std::vector<Edge*>> adjacencyMatrix;
    bool isDirected;
    int numberOfVertices;
    int numberOfEdges = 0;

    class AllEdgesIter: public Iterator<Edge> {
        GraphAsMatrix &owner;
        int row;
        int col;
        public:
            void next() {
                for(; row<owner.numberOfVertices; row++) {
                    for(++col; col<owner.numberOfVertices; col++) {
                        if(owner.adjacencyMatrix[row][col] != NULL)
                            return;
                    }
                    col = -1;
                }
            }
            AllEdgesIter(GraphAsMatrix &owner):owner(owner), row(0), col(-1) { next(); }
            bool IsDone() { return  row == owner.numberOfVertices; }
            Edge &operator*() { return *owner.SelectEdge(row,col); }
            void operator++(){ next(); }
    };

    class EmanEdgesIter: public Iterator<Edge> {
        GraphAsMatrix & owner;
        int row;
        int col;
        public:
            void next() {
                for(++col; col<owner.numberOfVertices; col++) {
                    if(owner.adjacencyMatrix[row][col] != NULL)
                        return;
                }
            }
            EmanEdgesIter(GraphAsMatrix &owner, int v):owner(owner), row(v), col(-1) {
                next();
            }
            bool IsDone() { return (col == owner.numberOfVertices); }
            Edge & operator*() { return *owner.SelectEdge(row,col); }
            void operator++(){ next(); }
    };

    public:
        Iterator<Edge> &Edges() { return *new AllEdgesIter(*this); }
        Iterator<Edge> &EmanatingEdges(int v) { return *new EmanEdgesIter(*this, v); }

        GraphAsMatrix (int n, bool b) {
            isDirected = b;
            numberOfVertices = n;
            for(int i=0; i<n; i++) {
                Vertex *temp = new Vertex(i);
                vertices.push_back(temp);
            }
            std::vector<Edge*> vec_NULL;
            for(int i=0; i<n; i++)
                vec_NULL.push_back(NULL);
            for(int i=0; i<n; i++)
                adjacencyMatrix.push_back(vec_NULL);
        }

        int NumberOfVertices() {
            return numberOfVertices;
        }

        bool IsDirected() {
            return isDirected;
        }

        int NumberOfEdges(){
            return numberOfEdges;
        }

        bool IsEdge(int u, int v) {
            if(adjacencyMatrix[u][v] != NULL)
                return true;
            return false;
        }

        void MakeNull() {
            adjacencyMatrix.clear();
        }

        void AddEdge (int u, int v) {
            if(!IsEdge(u,v)) {
                Edge* edge = new Edge(vertices[u], vertices[v]);
                adjacencyMatrix[u][v] = edge;
                if(!IsDirected()) {
                    edge = new Edge(vertices[v], vertices[u]);
                    adjacencyMatrix[v][u] = edge;
                }
                numberOfEdges++;
            }
        }

        void AddEdge (Edge* edge) {
            if(!IsEdge(edge->V0()->Number(), edge->V1()->Number())) {
                if(IsDirected())
                    adjacencyMatrix[edge->V0()->Number()][edge->V1()->Number()] = edge;
                else {
                    adjacencyMatrix[edge->V0()->Number()][edge->V1()->Number()] = edge;
                    Edge* edge_2 = new Edge(edge->V1(), edge->V0());
                    adjacencyMatrix[edge->V1()->Number()][edge->V0()->Number()] = edge_2;
                }
                numberOfEdges++;
            }
        }

        Edge* SelectEdge (int u, int v) {
            return adjacencyMatrix[u][v];
        }

        Vertex* SelectVertex(int v) {
            return vertices[v];
        }

        void Print_Edges() {
            auto &it = Edges();
            std::cout << "(Iterator) Krawędzie: ";
            while(!it.IsDone()) {
                std::cout << "("<< (*it).V0()->Number() << "," << (*it).V1()->Number() << ") ";
                ++it;
            }
            std::cout << std::endl;
        }

        void DFS_visitor(CountingVisitor *visitor, Vertex *v, std::vector<bool> &visited) {
            visitor->Visit(*v);
            visited[v->Number()] = true;
            auto &it = EmanatingEdges(v->Number());
            while(!it.IsDone()) {
                if(!visited[(*it).V1()->Number()])
                    DFS_visitor(visitor,(*it).V1(), visited);
                ++it;
            }
        }

        bool IsConnected() {
            CountingVisitor visitor;
            std::vector<bool> visited(numberOfVertices, false);
            DFS_visitor(&visitor, SelectVertex(0), visited);
            return (visitor.visited_Vertices == numberOfVertices);
        }
};

class Kruskal_labirynt {
    private:
        int width;
        int height;
        GraphAsMatrix *result;
        bool delay;
        unsigned int delay_time;

        std::vector<Edge*> Create_mesh() {
            std::vector<Edge*> edges;
            for(int i=0; i<height*width; i++) {
                if(i%width != 0) {
                    Vertex* V_0 = new Vertex(i-1);
                    Vertex* V_1 = new Vertex(i);
                    Edge* new_edge = new Edge(V_0, V_1);
                    edges.push_back(new_edge);
                    // std::cout << "Dodano : (" << i-1 << "," << i << ")\n";
                }
                if(i>=width) {
                    Vertex* V_0 = new Vertex(i-width);
                    Vertex* V_1 = new Vertex(i);
                    Edge* new_edge = new Edge(V_0, V_1);
                    edges.push_back(new_edge);
                    // std::cout << "Dodano : (" << i-m << "," << i << ")\n";
                }
            }
            return edges;
        }

        void RandomizeEdges(std::vector<Edge*> &edges) {
            srand(time(NULL));
            std::random_shuffle(edges.begin(), edges.end());
        }

        void CreateSets(std::vector<std::vector<int>> &All_sets) {
            for(int i=0; i<height*width; i++) {
                std::vector<int> set;
                set.push_back(i);
                All_sets.push_back(set);
                // std::cout << i << " " << All_sets[i][0] << std::endl;
            }
        }

        void ConnectVertices(Edge* edge, std::vector<std::vector<int>> &sets, GraphAsMatrix *result) {
            int a = edge->V0()->Number();
            int b = edge->V1()->Number();
            int vec_1, vec_2;
            for(int i=0; i<sets.size(); i++) {
                if(sets[i].size() != 0) {
                    bool found_a = std::find(sets[i].begin(), sets[i].end(), a) != sets[i].end();
                    bool found_b = std::find(sets[i].begin(), sets[i].end(), b) != sets[i].end();
                    if(found_a && found_b)
                        return;
                    if(found_a)
                        vec_1 = i;
                    if(found_b)
                        vec_2 = i;
                }
            }
            std::move(sets[vec_2].begin(), sets[vec_2].end(), std::back_inserter(sets[vec_1]));
            sets[vec_2].clear();
            result->AddEdge(edge);
            // std::cout << "Edge: " << edge->V0()->Number() << "," << edge->V1()->Number() << std::endl;
            if(delay) {
                system("clear");
                WypiszLabirynt(result);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_time));
            }
        }

        void WypiszLabirynt(GraphAsMatrix *result) {
            std::string walls_vertical, walls_horizontal;
            int vertices_number = result->NumberOfVertices();
            std::string edge_line = "+  +";
            for(int i=0; i<width-1; i++)
                edge_line += "--+";
            std::cout<< std::endl << edge_line << std::endl;;
            for(int i=0; i<vertices_number; i++) {
                walls_vertical += "  ";
                if(i%width != width-1) {
                    if(!result->IsEdge(i, i+1))
                        walls_vertical += "|";
                    else
                        walls_vertical += " ";
                }
                if(i < vertices_number-width) {
                    if(i !=0 && i%width == width-1) {
                        if(!result->IsEdge(i, i+width))
                            walls_horizontal += "--";
                        else
                            walls_horizontal += "  ";
                    } else {
                        if(!result->IsEdge(i, i+width))
                            walls_horizontal += "--+";
                        else
                            walls_horizontal += "  +";
                    }
                }
                if(i%width == width-1) {
                    std::cout << "|" << walls_vertical << "|" << std::endl;
                    // std::cout << walls_vertical << std::endl;
                    walls_vertical.clear();
                    if(i<=vertices_number-width) {
                        std::cout << "+" << walls_horizontal << "+" <<std::endl;
                        // std::cout << walls_horizontal << std::endl;
                        walls_horizontal.clear();
                    }
                }
            }
            std::reverse(edge_line.begin(), edge_line.end());
            std::cout << edge_line << std::endl;
        }

    public:
        Kruskal_labirynt(int n, int m, bool timed):height(n), width(m), delay(timed), delay_time(delay_time_), result(new GraphAsMatrix(height*width, false)) {
            std::vector<Edge*> edges = Create_mesh();
            RandomizeEdges(edges);
            std::vector<std::vector<int>> All_sets;
            CreateSets(All_sets);
            for(int i=edges.size()-1; edges.size() > 0; i--) {
                ConnectVertices(edges[i], All_sets, result);
                edges.pop_back();
            }
            if(!delay)
                WypiszLabirynt(result);
            std::cout << "\nCzy graf jest spójny : " << (result->IsConnected() ? "Tak" : "Nie") << std::endl;
            // result->Print_Edges();
        }
};

int main(int argc, char *argv[]) {
    if(argc < 4) {
        std::cout << "\nPodano nieprawidłową ilość argumentów!" << std::endl;
        return 0;
    }

    int n = std::atoi(argv[1]);
    int m = std::atoi(argv[2]);

    if(*argv[3] == '1')
        Kruskal_labirynt(n,m, true);
    else if(*argv[3] == '0')
        Kruskal_labirynt(n,m, false);
    else {
        std::cout << "\nWprowadzono nieprawidłowy 3 argument!" << std::endl;
        return 0;
    }

    return 0;
}














