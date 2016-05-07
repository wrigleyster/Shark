//===========================================================================
/*!
 * 
 *
 * \brief       Random Forest Trainer
 * 
 * 
 *
 * \author      K. N. Hansen, J. Kremer
 * \date        2011-2012
 *
 *
 * \par Copyright 1995-2015 Shark Development Team
 * 
 * <BR><HR>
 * This file is part of Shark.
 * <http://image.diku.dk/shark/>
 * 
 * Shark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Shark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Shark.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
//===========================================================================


#ifndef SHARK_ALGORITHMS_TRAINERS_RFTRAINER_H
#define SHARK_ALGORITHMS_TRAINERS_RFTRAINER_H

#include <shark/Core/DLLSupport.h>
#include <shark/Algorithms/Trainers/AbstractTrainer.h>
#include <shark/Models/Trees/RFClassifier.h>

#include <boost/unordered_map.hpp>
#include <set>

namespace shark {
/*!
 * \brief Random Forest
 *
 * Random Forest is an ensemble learner, that builds multiple binary decision trees.
 * The trees are built using a variant of the CART methodology
 *
 * The algorithm used to generate each tree based on the SPRINT algorithm, as
 * shown by J. Shafer et al.
 *
 * Typically 100+ trees are built, and classification/regression is done by combining
 * the results generated by each tree. Typically the a majority vote is used in the
 * classification case, and the mean is used in the regression case
 *
 * Each tree is built based on a random subset of the total dataset. Furthermore
 * at each split, only a random subset of the attributes are investigated for
 * the best split
 *
 * The node impurity is measured by the Gini criteria in the classification
 * case, and the total sum of squared errors in the regression case
 *
 * After growing a maximum sized tree, the tree is added to the ensemble
 * without pruning.
 *
 * For detailed information about Random Forest, see Random Forest
 * by L. Breiman et al. 2001.
 *
 * For detailed information about the SPRINT algorithm, see
 * SPRINT: A Scalable Parallel Classifier for Data Mining
 * by J. Shafer et al.
 */
class RFTrainer 
: public AbstractTrainer<RFClassifier, unsigned int>
, public AbstractTrainer<RFClassifier>,
  public IParameterizable
{

public:
	/// Construct and compute feature importances when training or not
	SHARK_EXPORT_SYMBOL RFTrainer(bool computeFeatureImportances = false, bool computeOOBerror = false);

	/// \brief From INameable: return the class name.
	std::string name() const
	{ return "RFTrainer"; }

	/// Train a random forest for classification.
	SHARK_EXPORT_SYMBOL void train(RFClassifier& model, ClassificationDataset const& dataset);

	/// Train a random forest for regression.
	SHARK_EXPORT_SYMBOL void train(RFClassifier& model, RegressionDataset const& dataset);

	/// Set the number of random attributes to investigate at each node.
	SHARK_EXPORT_SYMBOL void setMTry(std::size_t mtry);

	/// Set the number of trees to grow.
	SHARK_EXPORT_SYMBOL void setNTrees(std::size_t nTrees);

	/// Controls when a node is considered pure. If set to 1, a node is pure
	/// when it only consists of a single node.
	SHARK_EXPORT_SYMBOL void setNodeSize(std::size_t nTrees);

	/// Set the fraction of the original training dataset to use as the
	/// out of bag sample. The default value is 0.66.
	SHARK_EXPORT_SYMBOL void setOOBratio(double ratio);

	/// Return the parameter vector.
	RealVector parameterVector() const
	{
		RealVector ret(1); // number of trees
		init(ret) << (double)m_B;
		return ret;
	}

	/// Set the parameter vector.
	void setParameterVector(RealVector const& newParameters)
	{
		SHARK_ASSERT(newParameters.size() == numberOfParameters());
		setNTrees((size_t) newParameters[0]);
	}

protected:
	struct RFAttribute {
		double value;
		std::size_t id;
	};

	/// attribute table
	typedef std::vector < RFAttribute > AttributeTable;
	/// collecting of attribute tables
	typedef std::vector < AttributeTable > AttributeTables;

	/// ClassVector
	using ClassVector = UIntVector;

	/// Create attribute tables from a data set, and in the process create a count matrix (cAbove).
	/// A dataset with m features results in m attribute tables.
	/// [attribute | class/value | row id ]
	SHARK_EXPORT_SYMBOL void createAttributeTables(Data<RealVector> const& dataset, AttributeTables& tables);

	/// Create a count matrix as used in the classification case.
	SHARK_EXPORT_SYMBOL RFTrainer::ClassVector createCountMatrix(ClassificationDataset const& dataset) const;

	// Split attribute tables into left and right parts.
	SHARK_EXPORT_SYMBOL void splitAttributeTables(AttributeTables const& tables, std::size_t index, std::size_t valIndex, AttributeTables& LAttributeTables, AttributeTables& RAttributeTables);

	/// Build a decision tree for classification
	SHARK_EXPORT_SYMBOL CARTClassifier<RealVector>::TreeType buildTree(AttributeTables& tables, ClassificationDataset const& dataset, ClassVector& cFull, std::size_t nodeId, Rng::rng_type& rng);

	/// Builds a decision tree for regression
	SHARK_EXPORT_SYMBOL CARTClassifier<RealVector>::TreeType buildTree(AttributeTables& tables, RegressionDataset const& dataset, std::vector<RealVector> const& labels, std::size_t nodeId, Rng::rng_type& rng);

	/// comparison function for sorting an attributeTable
	SHARK_EXPORT_SYMBOL static bool tableSort(RFAttribute const& v1, RFAttribute const& v2);

	/// Generate a histogram from the count matrix.
	SHARK_EXPORT_SYMBOL RealVector hist(ClassVector const& countVector) const;

	/// Average label over a vector.
	SHARK_EXPORT_SYMBOL RealVector average(std::vector<RealVector> const& labels);

	/// Calculate the Gini impurity of the countMatrix
	SHARK_EXPORT_SYMBOL double gini(ClassVector const& countVector, std::size_t n) const;

	/// Total Sum Of Squares
	SHARK_EXPORT_SYMBOL double totalSumOfSquares(std::vector<RealVector>& labels, std::size_t from, std::size_t to, RealVector const& sumLabel);

	/// Generate random table indices.
	SHARK_EXPORT_SYMBOL std::set<std::size_t> generateRandomTableIndicies(Rng::rng_type& rng) const;

	/// Reset the training to its default parameters.
	SHARK_EXPORT_SYMBOL void setDefaults();

	/// Number of attributes in the dataset
	std::size_t m_inputDimension;

	/// size of labels
	union {
        ///Dimension of a label. Used in Regression
		std::size_t m_labelDimension;
		///Holds the number of distinct labels. Used in Classification
		std::size_t m_labelCardinality;
	};

	/// number of attributes to randomly test at each inner node
	std::size_t m_try;

	/// number of trees in the forest
	std::size_t m_B;

	/// number of samples in the terminal nodes
	std::size_t m_nodeSize;

	/// fraction of the data set used for growing trees
	/// 0 < m_OOBratio < 1
	double m_OOBratio;

	/// true if the trainer is used for regression, false otherwise.
	bool m_regressionLearner;

	// true if the feature importances should be computed
	bool m_computeFeatureImportances;

	// true if OOB error should be computed
	bool m_computeOOBerror;
};
}
#endif
