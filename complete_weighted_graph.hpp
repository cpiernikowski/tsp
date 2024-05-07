#ifndef COMPLETE_WEIGHTED_GRAPH_HPP
#define COMPLETE_WEIGHTED_GRAPH_HPP

#include <vector>
#include <cstdint>
#include <string> // debug?
#include <ranges>
#include <cassert>
#include <memory>
#include <iostream> // debug
#include "utility.hpp"

using weight_t = double;
using pheromones_t = double;
using vertex_t = std::size_t;
using path_t = std::vector<vertex_t>;
using connection_t = std::pair<vertex_t, vertex_t>;
using probability_t = float;

class CWGraph {
public:
    struct Edge {
        connection_t conn;
        weight_t w;
        pheromones_t ph;

        bool operator==(const Edge& other) const {
            return (conn.first == other.conn.first && conn.second == other.conn.second) ||
                   (conn.first == other.conn.second && conn.second == other.conn.first); 
        }

        bool operator==(const connection_t& other) const {
            return (conn.first == other.first && conn.second == other.second) ||
                   (conn.first == other.second && conn.second == other.first); 
        }

#ifdef MY_DEBUG
        operator std::string() const { // debug?
            using namespace std::string_literals;
            return
                "Pierwszy wierzchołek: "s + std::to_string(conn.first)
                + " Drugi wierzchołek: "s + std::to_string(conn.second)
                + " Waga: "s + std::to_string(w);
        }
#endif
    };

private:
    std::vector<Edge> edges;
    const std::size_t n; // zawsze ma byc wiadome przed konstrukcja, ile bedzie wierzcholkow

    decltype(n) n_of_edges() const noexcept {
        return util::factorial(n - 1);
    }

public:

    friend bool operator==(const connection_t& lhs, const connection_t& rhs) { //czy napewno potrzebne?
        return (lhs.first == rhs.first && lhs.second == rhs.second) ||
               (lhs.first == rhs.second && lhs.second == rhs.first);
    }
    
    CWGraph(std::size_t n_of_vertices) noexcept(noexcept(decltype(edges)()))
        : n{n_of_vertices} {
        assert(n > 0);
        edges.reserve(n_of_edges());
    }

    void add_edge(const connection_t& conn, weight_t w, pheromones_t ph) {
        edges.push_back(Edge{conn, w, ph});
        //edges.push_back(Edge{{conn.second, conn.first}, w, ph}); niepotrzebne?
    }

    void add_edge(const Edge& edge) {
        edges.push_back(edge);
        //edges.push_back(Edge{{edge.conn.second, edge.conn.first}, edge.w, edge.ph});
    }

#ifdef MY_DEBUG
    operator std::string() const {
        std::string out{};

        for (const auto& e : edges) {
            out += static_cast<std::string>(e) + std::string("\n");
        }

        return out;
    }
#endif

    //mozee niepotrzebne bedzie
    std::optional<weight_t> weight(const connection_t& c) const {
        auto&& elem = util::find_if_optional(edges, [&] (auto&& e) {
            return e == c;
        });

        return elem ? std::optional(elem->get().w) : std::nullopt;
    }

    std::optional<pheromones_t> pheromones(const connection_t& c) const {
        auto&& elem = util::find_if_optional(edges, [&] (auto&& e) {
            return e == c;
        });

        return elem ? std::optional(elem->get().ph) : std::nullopt;
    }

    std::optional<std::reference_wrapper<const Edge>> edge(const connection_t& c) const {
       return std::move(util::find_if_optional(edges, [&] (auto&& e) {
            return e == c;
       }));
    }
    //

    bool has_vertex(vertex_t v) const noexcept {
        return v <= n;
    }

    decltype(n) size() const noexcept {
        return n;
    }

    const auto& cedges() const noexcept {
        return edges;
    }
};

class Ant {
    static constexpr double ALPHA = 1.0;
    static constexpr double BETA = 1.0;

    path_t visited; // current in visited[visited.size() - 1]
    const CWGraph& graph;

    void travel(vertex_t v) {
        assert(graph.has_vertex(v));
        visited.push_back(v);
    }

public:
    Ant(const CWGraph& g)
        : graph(g) {
        assert(graph.size() > 0);
        visited.reserve(graph.size());
        visited.push_back(g.cedges()[0].conn.first);
    }

    vertex_t& current() {
        return visited[visited.size() - 1];
    }

    enum class Which : std::int8_t {
        FIRST = -1,
        INVALID = 0,
        SECOND = 1
    };

    static Which is_possible_connection(vertex_t v, connection_t conn) {
        // -1 jesli conn.first jest rowny, 1 jestli conn.second jest rowny, 0 jesli zaden
        std::cout << v << " " << conn.first << " " << conn.second << std::endl; // debug
        assert(conn.first != conn.second);
        using T = std::underlying_type_t<Which>;
        T out = 0;
        out += static_cast<T>(conn.first == v); // mozna polaczyc z conn.second
        out -= static_cast<T>(conn.second == v); // mozna polaczyc z conn.first
        return static_cast<Which>(out);
    }

    struct PotentialChoice {
        PotentialChoice() {

        }

        PotentialChoice(const CWGraph::Edge* e, Which w) // mozliwe ze niepotrzebne
            : edge{e}, which{w} {

        }

        static constexpr float PROBABILITY_NOT_YET_COMPUTED = -1.0f;

        const CWGraph::Edge* edge = nullptr;
        Which which = Which::INVALID;
        probability_t probability = PROBABILITY_NOT_YET_COMPUTED;

        void compute_probability() {
            
        }
    };

    void step() {
        const std::size_t n_of_choices = graph.size() - visited.size();
        std::cout << n_of_choices << std::endl;
        PotentialChoice* choices = new PotentialChoice[n_of_choices]{}; // ewentualnie zamienic na vector i vector.reserve(n_of_choices)
        const auto& edges = graph.cedges();

        std::size_t i = 0;
        std::size_t j = 0;
        for (; i < n_of_choices; ++j) {
            const auto is_possible = is_possible_connection(current(), edges[j].conn);
            if (is_possible != Which::INVALID) {
                if ((is_possible == Which::FIRST && util::contains(visited, edges[j].conn.first)) ||
                    (is_possible == Which::SECOND && util::contains(visited, edges[j].conn.second))) {
                    //ten warunek chyba git, ale cos smierdzialo wczesniej
                    continue;
                }
                choices[i].edge = &edges[j];
                choices[i].which = is_possible;
                ++i;
            }
        }

        for (int i = 0; i < n_of_choices; ++i) {
            std::cout << (std::string)*choices[i].edge << std::endl; // debug
        }

        

        // obliczyc dla kazdego wedlug prezentacji probabilities / mozliwe ze trzeba bedzie je zapisac w potentialchoice, pewnie ta
        // nastepnie wybrac konkretny za pomoca warunkopw z prezentacji i wykonac travel()
        // sprawdzic cczy to nie jest koneic wedrowki, porownanie visited.size() z graph.ccedges().size()? cos takiego jeszczep omysl
        // jesli jest koniec to wywolac sprzezenia zwrotne dodatnie i ujemne na grafie, zdefiniuj te zachwoanai w grafie raczej? graf wtedy bedzie non-const&
        // kazde sprzezenie zwrotne bedzie oddzielnei wyzwalane chyba dla kazdej mrowki, tak bedzie latwiej, dlatego moze jednak te zachowania nei w grafie, tylko w mrowce
        //todo: sprawdz na sztywno czy dobrze dodaje choices itp

    }
};

#endif