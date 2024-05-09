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
#include <cmath>
#include <random>
#include <algorithm>
#include <utility>

using weight_t = double;
using pheromones_t = double;
using vertex_t = std::size_t;
using path_t = std::vector<vertex_t>;
using connection_t = std::pair<vertex_t, vertex_t>;
using probability_t = double;

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
                "Pierwszy wierzcholek: "s + std::to_string(conn.first)
                + " Drugi wierzcholek: "s + std::to_string(conn.second)
                + " Waga: "s + std::to_string(w)
                + " Feromony: "s + std::to_string(ph);
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

template <double A = 1.0, double B = 1.0, double R = 0.2>
class Ant {
    static constexpr double ALPHA = A;
    static constexpr double BETA = B;
    static constexpr double RHO = R;

    path_t visited; // current in visited[visited.size() - 1]
    const CWGraph& graph;

    void travel(vertex_t v) {
        assert(graph.has_vertex(v));
        visited.push_back(v);
    }

public:
    const path_t& get_path() const noexcept { // debug?
        return visited;
    }

    Ant(const CWGraph& g)
        : graph(g) {
        assert(graph.size() > 0);
        visited.reserve(graph.size() + 1); // + 1 bo bedziemy wracac do pierwszego
        visited.push_back(graph.cedges()[0].conn.first);
    }

    vertex_t& current() {
        return visited[visited.size() - 1];
    }

    enum class Which : std::int8_t {
        FIRST = -1,
        INVALID = 0,
        SECOND = 1
    };

    static Which is_possible_connection(vertex_t v, const connection_t& conn) {
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

        static constexpr probability_t PROBABILITY_NOT_YET_COMPUTED = static_cast<probability_t>(-1);

        const CWGraph::Edge* edge = nullptr;
        Which which = Which::INVALID;
        probability_t probability = PROBABILITY_NOT_YET_COMPUTED;

        vertex_t vertex() const noexcept { // mozliwe ze bedzie niepotrzebne?
            assert(which != Which::INVALID);
            return which == Which::FIRST ? edge->conn.first : edge->conn.second;
        }

        auto operator<=>(const PotentialChoice& rhs) const noexcept {
            // zmienic na operator>? tylko on raczej potrzebny bo jedyne porownanie
            // odbywa sie przy wywolaniu std::sort(..., std::greater{});
            return probability <=> rhs.probability;
        }
    };

    struct PotentialChoices {
        PotentialChoice* choices;
        std::size_t n;

        PotentialChoices(std::size_t n_of_choices)
            : n{n_of_choices}, choices{new PotentialChoice[n_of_choices]{}}  {

        }

        ~PotentialChoices() {
            if (choices)
                delete[] choices;
        }

        PotentialChoice& operator[](std::size_t i) noexcept {
            assert(choices != nullptr && i < n);
            return choices[i];
        }

        void compute_probabilities() noexcept {
            assert(choices != nullptr);

            probability_t sum_denom{};
            for (std::size_t i = 0; i < n; ++i) {
                assert(choices[i].which != Which::INVALID && choices[i].edge != nullptr);
                sum_denom += std::pow(choices[i].edge->ph, ALPHA) * std::pow(1 / choices[i].edge->w, BETA);
            }

            for (std::size_t i = 0; i < n; ++i) {
                // oblicz dla kazdego PotentialChoice probability (licznik / sum_denom)
                choices[i].probability = (std::pow(choices[i].edge->ph, ALPHA) * std::pow(1 / choices[i].edge->w, BETA))
                                         / sum_denom;
            }
        }

        vertex_t vertex_to_visit() {
            assert(n > 0);
            std::sort(choices, std::next(choices, n), std::greater{}); // na pewno nie n+1?
            static std::random_device rd;
            static std::mt19937 mt(rd());
            static_assert(std::is_floating_point_v<probability_t>);
            static std::uniform_real_distribution<probability_t> dis(0.0, 1.0);
            const probability_t r = dis(mt);

            auto sum = [this] (std::size_t i) -> probability_t {
                probability_t out{};
                for (; i < n; ++i) {
                    out += choices[i].probability;
                }
                return out;
            };
            
            probability_t earlier_s = sum(0);
            for (int i = 1; i < n; ++i) {
                probability_t current_s = sum(i);
            
                if (r > current_s && r <= earlier_s) {
                    return choices[i - 1].vertex();
                }

                earlier_s = current_s;
            }

            return choices[n - 1].vertex();
        }
    };

    void step() {
        assert(visited.size() < graph.size());
        const std::size_t n_of_choices = graph.size() - visited.size();
        std::cout << n_of_choices << std::endl; // debug
        PotentialChoices choices(n_of_choices);
        const auto& edges = graph.cedges();

        // populate potential choices, przeniesc do tamtej klasy jako funkcja?
        std::size_t i = 0;
        std::size_t j = 0;
        for (; i < n_of_choices; ++j) { // <- ++j not ++i!
            const auto is_possible = is_possible_connection(current(), edges[j].conn);
            if (is_possible != Which::INVALID) {
                if ((is_possible == Which::FIRST && util::contains(visited, edges[j].conn.first)) ||
                    (is_possible == Which::SECOND && util::contains(visited, edges[j].conn.second))) {
                    continue;
                }
                choices[i].edge = &edges[j];
                choices[i].which = is_possible;
                ++i;
            }
        }

        choices.compute_probabilities(); // sprawdzone
#ifdef MY_DEBUG
        std::cout << "Prawdopodobienstwa:" << std::endl;
        for (int i = 0; i < n_of_choices; ++i) {
            std::cout << choices[i].probability << std::endl;
        }
        std::cout << "Koniec prawdopodobienstw" << std::endl;
#endif
        travel(choices.vertex_to_visit());

        //todo: gdy nie ma juz zadnych do odwiedzenia miast wybrac sie do miasta pierwszego (visited[0]) i zakocnzyc wedrowke

        // sprawdzic cczy to nie jest koneic wedrowki, porownanie visited.size() z graph.ccedges().size()? cos takiego jeszczep omysl/
        // jesli jest koniec to wywolac sprzezenia zwrotne dodatnie i ujemne na grafie, zdefiniuj te zachwoanai w grafie raczej? graf wtedy bedzie non-const&
        // kazde sprzezenie zwrotne bedzie oddzielnei wyzwalane chyba dla kazdej mrowki, tak bedzie latwiej, dlatego moze jednak te zachowania nei w grafie, tylko w mrowce
    }
};

#endif