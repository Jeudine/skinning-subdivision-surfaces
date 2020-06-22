#include "Mesh.h"
#include <set>
#include <eigen3/Eigen/SparseCore>

using namespace std;
using namespace Eigen;
typedef Eigen::SparseMatrix<float> SpMat;

void Mesh::subdivide() {

    vector<Triangle> new_triangleIndices;
    vector<Uvec2> neighbours; //to know if the odd vertex already exists
    vector<Uvec2> neighbour_triangles; // to move odd vertices


    map<unsigned int, vector <unsigned int>> even_n;
    vector <Uvec2>::iterator it;
    unsigned int counter = vertices.size();
    const unsigned int len_vertices = counter;
    Vertex x, y ,z;
    Triangle new_triangle;

    int triangle_count = 0;
    vector<set <unsigned int> > neighbour_vertices(len_vertices); //to move even vertices

    auto create_odd_vertex=[&](unsigned int & odd_vertex, unsigned int even_vertex1, unsigned int even_vertex2) -> void {
        unsigned int tmp;
        if(even_vertex1 > even_vertex2) {
            tmp = even_vertex1;
            even_vertex1 = even_vertex2;
            even_vertex2 = tmp;
        }

        it = find(neighbours.begin(), neighbours.end(), Uvec2(even_vertex1, even_vertex2));

        if(it == neighbours.end()) {
            odd_vertex = counter;
            counter ++;
            neighbour_triangles.push_back(Uvec2(triangle_count, numeric_limits<unsigned int>::infinity()));
            neighbours.push_back(Uvec2(even_vertex1, even_vertex2));
        } else {
            neighbour_triangles[std::distance(neighbours.begin(), it)][1] = triangle_count;
            odd_vertex = std::distance(neighbours.begin(), it) + vertices.size();
        }
    };

    for(const Triangle & triangle : triangles) {

        create_odd_vertex(new_triangle[0], triangle[0], triangle[1]);
        create_odd_vertex(new_triangle[1], triangle[1], triangle[2]);
        create_odd_vertex(new_triangle[2], triangle[2], triangle[0]);

        neighbour_vertices[triangle[0]].insert(triangle[1]);
        neighbour_vertices[triangle[0]].insert(triangle[2]);
        neighbour_vertices[triangle[1]].insert(triangle[2]);
        neighbour_vertices[triangle[1]].insert(triangle[0]);
        neighbour_vertices[triangle[2]].insert(triangle[0]);
        neighbour_vertices[triangle[2]].insert(triangle[1]);

        new_triangleIndices.push_back(new_triangle);
        new_triangleIndices.push_back(Triangle(triangle[0], new_triangle[0], new_triangle[2]));
        new_triangleIndices.push_back(Triangle(triangle[1], new_triangle[1], new_triangle[0]));
        new_triangleIndices.push_back(Triangle(triangle[2], new_triangle[2], new_triangle[1]));
        triangle_count ++;

    }


    // move
    std::vector< std::map< unsigned int, float > > old_coeffs = coeffs;
    coeffs = vector<std::map< unsigned int, float > >(counter);

    auto addCoeff=[&](unsigned int vertex, coeff k) -> void {
        for(auto const & it : old_coeffs[k.vertex]) {
            coeffs[vertex][it.first] += it.second * k.lambda;
        }
    };

    //move odd
    vector<Vertex> odd_vertexPositions;
    unsigned int len = counter - len_vertices;
    unsigned int triangle1[3];
    unsigned int triangle2[3];
    unsigned int A, B, C, D;

    for(unsigned int i = 0; i < len; i++) {
        if (neighbour_triangles[i][1] != numeric_limits<unsigned int>::infinity()) {
            triangle1[0] = triangles[neighbour_triangles[i][0]][0];
            triangle2[0] = triangles[neighbour_triangles[i][1]][0];
            ;
            triangle1[1] = triangles[neighbour_triangles[i][0]][1];
            triangle2[1] = triangles[neighbour_triangles[i][1]][1];
            ;
            triangle1[2] = triangles[neighbour_triangles[i][0]][2];
            triangle2[2] = triangles[neighbour_triangles[i][1]][2];

            sort(triangle1, triangle1+3);
            sort(triangle2, triangle2+3);

            if(triangle1[1] == triangle2[1]) {
                B = triangle1[1];
                if (triangle1[0] == triangle2[0]) {
                    C = triangle1[0];
                    A = triangle1[2];
                    D = triangle2[2];
                } else {
                    C = triangle1[2];
                    A = triangle1[0];
                    D = triangle2[0];
                }
            } else if(triangle1[1] == triangle2[0]) {
                B = triangle1[1];
                A = triangle1[0];
                C = triangle1[2];
                if (triangle1[2] == triangle2[1]) {
                    D = triangle2[2];
                } else {
                    D = triangle2[1];
                }
            } else if(triangle1[1] == triangle2[2]) {
                B = triangle1[1];
                A = triangle1[2];
                C = triangle1[0];

                if (triangle1[0] == triangle2[1]) {
                    D = triangle2[0];
                } else {
                    D = triangle2[1];
                }
            } else {
                A = triangle1[1];
                B = triangle1[0];
                C = triangle1[2];
                D = triangle2[1];
            }
            odd_vertexPositions.push_back((vertices[A] + vertices[D])/8.f +(3.f/8.f)*(vertices[B] + vertices[C]));
            addCoeff( len_vertices + i, {A, 1.f/8.f});
            addCoeff( len_vertices + i, {D, 1.f/8.f});
            addCoeff( len_vertices + i, {B, 3.f/8.f});
            addCoeff( len_vertices + i, {C, 3.f/8.f});
        } else {
            odd_vertexPositions.push_back((vertices[neighbours[i][0]] + vertices[neighbours[i][1]])/2.f);
            addCoeff(len_vertices + i, {neighbours[i][0], 1.f/2.f});
            addCoeff(len_vertices + i, {neighbours[i][1], 1.f/2.f});
        }

    }

    // move even
    int n;
    Vertex res;
    float alphan = 3.f/8.f;
    float alpha3 = 3.f/16.f;
    std::vector< Vertex > old_vertices = vertices;
    for (unsigned int i = 0; i<len_vertices ; i++) {
        n = neighbour_vertices[i].size();
        if (n == 2) {
            res = Vertex(0.f,0.f,0.f);
            for(unsigned int j : neighbour_vertices[i]) {
                res += 1.f/8.f * old_vertices[j];
                addCoeff(i, {j, 1.f/8.f});
            }
            res += 3.f/4.f * vertices[i];
            addCoeff(i, {i, 3.f/4.f});
            vertices[i] = res;

        } else if (n == 3) {
            res = Vertex(0.f,0.f,0.f);
            for(unsigned int j : neighbour_vertices[i]) {
                res += alpha3 *  old_vertices[j];
                addCoeff(i, {j, alpha3});
            }
            res += (1 - 3.f*alpha3) * vertices[i];
            addCoeff(i, {i,1 - 3.f*alpha3});
            vertices[i] = res;
        }
        else {
            res = Vertex(0.f,0.f,0.f);
            for(unsigned int j : neighbour_vertices[i]) {
                res += alphan/(float)n *  old_vertices[j];
                addCoeff(i, {j, alphan/(float)n});
            }
            res += (1-alphan) * vertices[i];
            addCoeff(i, {i,1 - alphan});
            vertices[i] = res;
        }
    }
    vertices.insert(vertices.end(), odd_vertexPositions.begin(), odd_vertexPositions.end());
    triangles = new_triangleIndices;
    vertices_no = vertices;
    colors = std::vector<float[3]> (vertices.size());
    for(unsigned int i = 0; i<vertices.size(); i++)
        for(unsigned int k = 0; k<3; k++)
            colors[i][k] = 0.7;
}

void Mesh::reset() {
    vertices = resetVertices;
    vertices_no = resetVertices;
    controlVertices = resetVertices;
    triangles = basicTriangles;
    coeffs = vector<std::map< unsigned int, float > >(controlVertices.size());
    colors = vector<float[3]>(controlVertices.size());
    for(unsigned int i = 0; i<controlVertices.size(); i++) {
        coeffs[i][i] = 1.f;
        for (unsigned int j = 0; j<3; j++)
            colors[i][j] = 0.7;
    }
}

void Mesh::computeQis(const std::vector<GausCoeff>gCoeffs) {
    const unsigned int len_coeffs = coeffs.size();
    const unsigned int len_basic = controlVertices.size();
    const unsigned int len_gCoeffs = gCoeffs.size();


    //compute dp
    const unsigned int len_triangles = triangles.size();
    vector<float> dp(len_coeffs, 0);
    float area;
    for(unsigned int k = 0; k < len_triangles; k++) {
        area = area_d3(k);
        dp[triangles[k][0]] += area;
        dp[triangles[k][1]] += area;
        dp[triangles[k][2]] += area;
    }

    //compute Qis
    SpMat Ap(len_basic,len_basic);
    std::map<unsigned int, float>::const_iterator j;
    MatrixXf A = MatrixXf::Zero(len_basic, len_basic);

    Qis = vector<MatrixXf>(len_gCoeffs);
    vector<MatrixXf> tis = vector<MatrixXf>(len_gCoeffs);
    vector<float> wis(len_gCoeffs);

    for(unsigned int i = 0; i < len_gCoeffs; i++) {
        tis[i] = MatrixXf::Zero(len_basic, len_basic);
    }

    for (unsigned int k = 0; k < len_coeffs; k++) {
        float normWis = 0;
        Ap.setZero();
        for (auto const & i : coeffs[k]) {
            j = coeffs[k].cbegin();
            while(j->first < i.first) {
                Ap.insert(i.first, j->first) = i.second * j->second;
                Ap.insert(j->first, i.first) = i.second * j->second;
                j++;
            }
            Ap.insert(i.first, i.first) = i.second * i.second;
        }
        if(len_gCoeffs > 1) {
            colors[k][0] = 0;
            colors[k][1] = 0;
            colors[k][2] = 0;
        }
        for(unsigned int i = 0; i < len_gCoeffs; i++) {
            wis[i] = exp(-(vertices[k] - gCoeffs[i].mean).sqrnorm()/(2*gCoeffs[i].variance));
            normWis += wis[i];
            if(len_gCoeffs > 1)
                colors[k][i%3] += wis[i];
        }
        for(unsigned int i = 0; i < len_gCoeffs; i++) {
            tis[i] += wis[i]/normWis * dp[k] * Ap;
        }
        A += dp[k] * Ap;
    }

    MatrixXf A_1 = A.inverse();

    MatrixXf C(4, len_basic);

    for(unsigned int j = 0; j < len_basic; j++) {
        C(0,j) = controlVertices[j][0];
        C(1,j) = controlVertices[j][1];
        C(2,j) = controlVertices[j][2];
        C(3,j) = 1;
    }

    for(unsigned int i = 0; i < len_gCoeffs; i++) {
        Qis[i] = C  * tis[i] * A_1;
    }
}

void Mesh::transform(const vector<MatrixXf> & T) {
    unsigned int len_basic = controlVertices.size();
    Eigen::MatrixXf C = MatrixXf::Zero(4, len_basic);
    for (unsigned int i = 0; i < T.size(); i++) {
        C += T[i]*Qis[i];
    }
    for (unsigned int j = 0; j < len_basic; j++) {
        controlVertices[j] = Vertex(C(0,j), C(1,j), C(2,j));
    }
    unsigned int n = coeffs.size();
    Vertex tmp;
    for(unsigned int i = 0; i < n; i++) {
        tmp = {0, 0, 0};
        for (auto const & it : coeffs[i]) {
            tmp += it.second * controlVertices[it.first];
        }
        vertices[i] = tmp;
    }
}


void Mesh::transform_Basic(const vector<MatrixXf> & T, const std::vector<GausCoeff>gCoeffs) {
    const unsigned int len_coeffs = coeffs.size();
    const unsigned int len_gCoeffs = gCoeffs.size();
    float wis;
    for (unsigned int k = 0; k < len_coeffs; k++) {
        Eigen::VectorXf tmp_e = Eigen::VectorXf::Zero(4);
        Eigen::VectorXf tmp = Eigen::VectorXf::Zero(4);
        float normWis = 0;
        for(unsigned int i = 0; i < len_gCoeffs; i++) {
            wis = exp(-(vertices_no[k] - gCoeffs[i].mean).sqrnorm()/(2*gCoeffs[i].variance));
            normWis += wis;
            for(unsigned int j = 0; j<3; j++)
                tmp_e(j) = vertices_no[k][j];
            tmp_e(3) = 1;
            tmp += wis * T[i] * tmp_e;
        }
        vertices[k] = {tmp(0)/normWis, tmp(1)/normWis, tmp(2)/normWis};
    }
}
