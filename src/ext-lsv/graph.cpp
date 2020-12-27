
#include "ext-lsv/graph.h"

namespace lsv
{

Node* neighbor(const Node* n, const Edge* e)
{
    /// return the opposite neighbor
    if( n==e->n1 ) return e->n2;
    return e->n1;
}

bool is_neighbor(const Node* n1, const Node* n2)
{
    /// check neighbor
    for( Node* n : n1->neighbors )
        if( n==n2 ) return true;
    return false;
}

std::ostream& operator<<(std::ostream& os, const Edge* e)
{
    if(e->var!=-1)
        os << e->var << " ";
    else
        for( int var : e->vars )
            os << var << " ";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::deque<Edge*> path)
{
    for( Edge* e : path )
        os << e << "-- ";
    return os;
}

Edge* Graph::find_extendable_edge( Node* node )
{
    for( Edge* e : node->edges )
        if( e->visited ==0 ) return e;
    assert( 0 && "can not find extendable edge");
    return nullptr;
}

bool Graph::find_cycle(Node* node)
{
    /// find a cycle through _out, put in path vector
    if( node )
    {
        if( node==_out && path.size()>2  )
        {
            return true;
        }
        else if( node->visited == 1 )
        {
            return false;
        }
        else
        {
            node->visited = 1;
            for( Edge* e : node->edges )
            {
                path.push_back(e);
                if( find_cycle( neighbor(node, e) ) ) return true;
                path.pop_back();
            }
        }
    }
    return true;
}

Face* Graph::find_face( Node* node, Node* target, Face* current_face, std::deque<Edge*>& path )
{
    /// find a path from node to target, that all edges belong to the same face
    if( node==target ) return current_face;

    Face* ret;
    if( current_face==nullptr )
    {
        /// starting node, try all embedded edges
        for( Edge* e : node->edges )
        {
            if( e->visited==1 ) continue;
            e->visited = 1;
            if( e->f1!=nullptr )
            {
                path.push_back(e);
                ret = find_face( neighbor(node,e), target, e->f1, path );
                if( ret!=nullptr ) return ret;
                path.pop_back();
            }
            if( e->f2!=nullptr )
            {
                path.push_back(e);
                ret = find_face( neighbor(node,e), target, e->f2, path );
                if( ret!=nullptr ) return ret;
                path.pop_back();
            }
            e->visited = 0;
        }
    }
    else
    {
        /// follow current_face
        for( Edge* e : node->edges )
        {
            if( e->visited==1 ) continue;
            e->visited = 1;
            if( e->f1==current_face )
            {
                path.push_back(e);
                ret = find_face( neighbor(node,e), target, e->f1, path );
                if( ret!=nullptr ) return ret;
                path.pop_back();
            }
            if( e->f2==current_face )
            {
                path.push_back(e);
                ret = find_face( neighbor(node,e), target, e->f2, path );
                if( ret!=nullptr ) return ret;
                path.pop_back();
            }
            e->visited = 0;
        }
    }
    return nullptr;
}

void Graph::initial_face()
{
    /// initialize two faces by the path
    for( auto* ptr : _faces ) delete ptr;
    _faces.clear();

    Face* f1 = new Face(0);
    Face* f2 = new Face(1);
    _faces.push_back(f1);
    _faces.push_back(f2);
    for( Edge* e : path )
    {
        e->f1 = f1;
        e->f2 = f2;
        f1->edges.push_back(e);
        f2->edges.push_back(e);
        e->n1->embedded = 1;
        e->n2->embedded = 1;
    }
}

void Graph::embed_all_edges()
{
    /// for all unembedded edges
    for( Edge* e : _edges )
    {
        if( e->f1 != nullptr ) continue;
        path.clear();
        clear_edge_visited();

        e->visited = 1;
        path.push_back(e);
        extend_path();

        slice_by_path();
    }
}

void Graph::extend_path()
{
    /// extended path until its two ends are embedded nodes
    path_front = path.front()->n1;
    path_back = path.front()->n2;
    while( true )
    {
        if( path_front->embedded == 0 )
        {
            Edge* edge = find_extendable_edge( path_front );
            edge->visited = 1;
            path.push_front(edge);
            path_front->embedded = 1;
            path_front = neighbor(path_front, edge);
        }
        else if( path_back->embedded == 0 )
        {
            Edge* edge = find_extendable_edge( path_back );
            edge->visited = 1;
            path.push_back(edge);
            path_back->embedded = 1;
            path_back = neighbor(path_back, edge);
        }
        else
        {
            break;
        }
    }
}

void Graph::slice_by_path()
{
    /// slice a face into two by the path
    clear_edge_visited();

    std::deque<Edge*> inner_path;
    Face* old_face = find_face(path_front, path_back, nullptr, inner_path);
    Face* new_face = new Face(_faces.size());
    _faces.push_back(new_face);


    std::cout << "path = " << path << std::endl;
    std::cout << "inner_path = " << inner_path << std::endl;

    /// remove edges from old face
    for( Edge* e : inner_path )
    {
        old_face->edges.remove(e);
        new_face->edges.push_back(e);
        if( e->f1 == old_face ) e->f1 = new_face;
        if( e->f2 == old_face ) e->f2 = new_face;
    }
    for( Edge* e : path )
    {
        old_face->edges.push_back(e);
        new_face->edges.push_back(e);
        e->f1 = old_face;
        e->f2 = new_face;
    }
}

void Graph::embed()
{
    /// embed graph onto plane, generate all faces
    reset_workspace();
    find_cycle(_out);
    initial_face();
    embed_all_edges();

}

Edge* Graph::find_edge(const Node* n1, const Node* n2)
{
    /// check if edge (n1,n2) exists
    for( Edge* e : _edges )
        if( e && ((e->n1==n1 && e->n2==n2) || (e->n1==n2 && e->n2==n1)) ) return e;
    return nullptr;
}

Edge* Graph::add_edge(int var, Node* n1, Node* n2)
{
    /// add new edge (non-dup edge style)
    Edge* e = find_edge(n1, n2);
    if( e )
    {
        e->vars.push_back(var);
    }
    else
    {
        e = new Edge(n1,n2);
        e->vars.push_back(var);
        _edges.push_back(e);
        /// update node connectivity
        n1->edges.push_back(e);
        n1->neighbors.push_back(n2);
        n2->edges.push_back(e);
        n2->neighbors.push_back(n1);
    }
    return e;
}

void Graph::add_ext_edge()
{
    assert( _gnd!=nullptr && _out!=nullptr );
    _ext_edge = new Edge(_out,_gnd);
    _ext_edge->vars.push_back(0);

    _edges.push_back(_ext_edge);
    _out->edges.push_back(_ext_edge);
    _gnd->edges.push_back(_ext_edge);

    if( !is_neighbor(_out,_gnd) )
    {
        _out->neighbors.push_back(_gnd);
        _gnd->neighbors.push_back(_out);
    }
}

void Graph::dump(std::ostream& os)
{
    os << "node: edges" << std::endl;
    for( Node* n : _nodes )
    {
        if( n )
        {
            os << " " << n->idx << " : ";
            for( Edge* e : n->edges )
            {
                os << e << ", ";
            }
        }
        os << std::endl;
    }

    os << "node: neighbors" << std::endl;
    for( Node* n : _nodes )
    {
        if( n )
        {
            os << " " << n->idx << " : ";
            for( Node* neighbor : n->neighbors )
                os << neighbor->idx << " ";
        }
        os << std::endl;
    }

    os << "edge (n1 , n2) : vars : f1, f2" << std::endl;
    for( Edge* e : _edges )
    {
        if( e )
        {
            os << "( " << e->n1->idx << " , " << e->n2->idx << " ) : " ;
            os << e << ", ";
            os << ": ";
            if( e->f1 ) os << e->f1->idx << " , ";
            else os << "null , ";
            if( e->f2 ) os << e->f2->idx << "";
            else os << "null";
        }
        os << std::endl;
    }

    os << "face : edges" << std::endl;
    for( Face* f : _faces )
    {
        if( f )
        {
            os << " " << f->idx << " : " ;
            for( Edge* e : f->edges )
                os << e << ", ";
        }
        os << std::endl;
    }
}

void Graph::read_mos_network(const char* input_file)
{
    std::cout << "read mos netlist : " << input_file << std::endl;
    std::ifstream ifs(input_file);
    assert( ifs.is_open() );

    int num_node, num_edge;
    ifs >> num_node >> num_edge;

    _nodes.resize(num_node,nullptr);
    _edges.resize(num_edge,nullptr);

    for( int i=0; i<num_node; ++i )
    {
        _nodes[i] = new Node(i);
    }

    for( int i=0; i<num_edge; ++i )
    {
        char type;
        int var, n1, n2;
        ifs >> type >> var >> n1 >> n2;
        _edges[i] = new Edge(var, _nodes[n1], _nodes[n2]);
        _nodes[n1]->edges.push_back(_edges[i]);
        _nodes[n2]->edges.push_back(_edges[i]);
        _nodes[n1]->neighbors.push_back(_nodes[n2]);
        _nodes[n2]->neighbors.push_back(_nodes[n1]);
    }

    _gnd = _nodes[0];
    _out = _nodes[1];

    ifs.close();
}

void Graph::read_mos_network_no_dup(const char* input_file)
{
    std::cout << "read mos netlist : " << input_file << std::endl;
    std::ifstream ifs(input_file);
    assert( ifs.is_open() );

    int num_node, num_edge;
    ifs >> num_node >> num_edge;

    /// allocate for nodes
    _nodes.resize(num_node);
    for( int i=0; i<num_node; ++i )
        _nodes[i] = new Node(i);

    /// read all edges
    for( int i=0; i<num_edge; ++i )
    {
        char type;
        int var, n1, n2;
        ifs >> type >> var >> n1 >> n2;
        add_edge(var,_nodes[n1],_nodes[n2]);
    }

    _gnd = _nodes[0];
    _out = _nodes[1];

    ifs.close();
}

}   /// end of namespace lsv
