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

    std::size_t n_of_edges() const noexcept {
        return util::factorial(n - 1);
    }

public:
    
    CWGraph(std::size_t n_of_vertices)
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

    // zakladamy, ze te 3 funkcje będą wywołane z istniejącym połączeniem jako argument, nieistniejące połączenia nie są obsługiwane jako błąd w tych funkcjach,
    // z zdefiniowanym NDEBUG przed błędem chroni assert
    weight_t weight(const connection_t& c) const {
        auto&& res = std::ranges::find_if(edges, [&c] (const auto& e) {
            return e == c;
        });

        assert(res != edges.end());

        return res->w;
    }

    pheromones_t pheromones(const connection_t& c) const {
        auto&& res = std::ranges::find_if(edges, [&c] (const auto& e) {
            return e == c;
        });

        assert(res != edges.end());

        return res->ph;
    }

    const Edge& edge(const connection_t& c) const {
        auto&& res = std::ranges::find_if(edges, [&c] (const auto& e) {
            return e == c;
        });

        assert(res != edges.end());

        return *res;
    }
    //

    bool has_vertex(vertex_t v) const noexcept {
        return v <= n;
    }

    auto size() const noexcept {
        return n;
    }

    const auto& cedges() const noexcept {
        return edges;
    }
};

class Ant {
    static constexpr double ALPHA = 1;
    static constexpr double BETA = 1;
    static constexpr double RHO = 0.2; // ewentualnie zaimplementowac definiowanie tych wartosci przez uzytkownika

    path_t visited{}; // current in visited[visited.size() - 1]
    const CWGraph& graph;
    bool finished = false;
    weight_t p_weight{};

    void travel(vertex_t v) {
        assert(graph.has_vertex(v));
        p_weight += graph.weight(connection_t{visited.back(), v});
        visited.push_back(v);
    }

public:

    weight_t path_weight() const noexcept {
        return p_weight;
    }

    const path_t& get_path() const noexcept { // debug?
        return visited;
    }

    Ant(const CWGraph& g)
        : graph(g) {
        assert(graph.size() > 0);
        visited.reserve(graph.size() + 1); // + 1 bo bedziemy wracac do pierwszego
        visited.push_back(graph.cedges().at(0).conn.first);
    }

    vertex_t& current() { // bez refa?
        return visited[visited.size() - 1];
    }

    enum class Which : std::int8_t {
        FIRST,
        SECOND,
        INVALID
    };

    static Which is_possible_connection(vertex_t v, const connection_t& conn) noexcept {
        //todo: zmienic nazwe na taka ktora nie bedzie brzmiala jakby funkcja zwracala boola
        Which out;
        if (conn.first == v) {
            out = Which::SECOND;
        } else if (conn.second == v) {
            out = Which::FIRST;
        } else {
            out = Which::INVALID;
        }
        return out;
    }

    struct PotentialChoice {
        PotentialChoice() {

        }

        PotentialChoice(const CWGraph::Edge* e, Which w) // mozliwe ze niepotrzebne
            : edge{e}, which{w} {

        }

        static constexpr probability_t PROBABILITY_NOT_YET_COMPUTED = static_cast<probability_t>(-1); // mozliwe ze niepotrzebne

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
        std::size_t n;
        PotentialChoice* choices;

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

        const PotentialChoice& operator[](std::size_t i) const noexcept {
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
            static std::random_device rd;
            static std::mt19937 mt(rd());
            static_assert(std::is_floating_point_v<probability_t>);
            static std::uniform_real_distribution<probability_t> dis(0.0, 1.0);

            const probability_t r = dis(mt);
            

            
            // sprobuj podejsc inaczej do tej sumy - 
            // moze najpierw obliczyc cala, a z kolejnymi iteracjami petli tej nizszej
            // odejmowac probability nastepnego? cos w tym stylu pokmin
            auto sum = [this] (std::size_t i) -> probability_t {
                probability_t out{};
                for (; i < n; ++i) {
                    out += choices[i].probability;
                }
                return out;
            };

            std::sort(choices, std::next(choices, n), std::greater{});

            probability_t earlier_s = sum(0);
            for (std::size_t i = 1; i < n; ++i) {
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
        assert(visited.size() <= graph.size());

        if (visited.size() == graph.size()) {
            // odwiedzono juz wszystkei miasta - trzeba wrocic do pierwszego
            travel(graph.cedges().at(0).conn.first);

            // zasygnalizowac, ze praca zostala skonczona (gdy wsztyskie morwki skoncza iteracje (znajda cala droge) to wtedy dopiero sprzezenia)
            finished = true;
            return;
        }

        const std::size_t n_of_choices = graph.size() - visited.size();
        std::cout << n_of_choices << std::endl; // debug
        PotentialChoices choices(n_of_choices);
        const auto& edges = graph.cedges();

        // populate potential choices, przeniesc do tamtej klasy jako funkcja?
        std::size_t i = 0;
        std::size_t j = 0;
        for (; i < n_of_choices; ++j) { // <- ++j not ++i!
            const Which is_possible = is_possible_connection(current(), edges[j].conn);
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
        for (std::size_t i = 0; i < n_of_choices; ++i) {
            std::cout << choices[i].probability << std::endl;
        }
        std::cout << "Koniec prawdopodobienstw" << std::endl;
#endif
        travel(choices.vertex_to_visit());

        // sprawdzic cczy to nie jest koneic wedrowki, porownanie visited.size() z graph.ccedges().size()? cos takiego jeszczep omysl/
        // jesli jest koniec to wywolac sprzezenia zwrotne dodatnie i ujemne na grafie, zdefiniuj te zachwoanai w grafie raczej? graf wtedy bedzie non-const&
        // kazde sprzezenie zwrotne bedzie oddzielnei wyzwalane chyba dla kazdej mrowki, tak bedzie latwiej, dlatego moze jednak te zachowania nei w grafie, tylko w mrowce
    }

    void travel_whole_journey() {
        while (!finished) {
            step();
        }
    }
};

class AntColony {
    // synchronizuje działanie mrówek, potrzebne ze względu na to, że sprzezenia zwrotne uruchamiane są dopiero,
    // gdy wszystkie mrówki ukoncza swoja prace podczas jednej iteracji
    // skladowe to std::vector<Ant>; licznik, ktory bedzie sprawdzal, czy juz zostala wykonana zadana ilosc iteracji; buffer na dotychczas najkrotsza droge
};

#endif