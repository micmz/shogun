/*
 * Copyright (c) The Shogun Machine Learning Toolbox
 * Written (w) 2016 Soumyajit De
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the Shogun Development Team.
 */

#include <algorithm>
#include <shogun/base/some.h>
#include <shogun/lib/SGMatrix.h>
#include <shogun/lib/SGVector.h>
#include <shogun/features/Features.h>
#include <shogun/features/DenseFeatures.h>
#include <shogun/kernel/Kernel.h>
#include <shogun/kernel/GaussianKernel.h>
#include <shogun/mathematics/Math.h>
#include <shogun/mathematics/eigen3.h>
#include <shogun/statistical_testing/MMD.h>
#include <shogun/statistical_testing/internals/mmd/BiasedFull.h>
#include <shogun/statistical_testing/internals/mmd/UnbiasedFull.h>
#include <shogun/statistical_testing/internals/mmd/UnbiasedIncomplete.h>
#include <shogun/statistical_testing/internals/mmd/WithinBlockPermutation.h>
#include <gtest/gtest.h>

using namespace shogun;
using namespace Eigen;

TEST(WithinBlockPermutation, biased_full)
{
	const index_t dim=2;
	const index_t n=6;
	const index_t m=4;

	using operation=std::function<float64_t(SGMatrix<float64_t>)>;

	SGMatrix<float64_t> data(dim, n+m);
	std::iota(data.matrix, data.matrix+dim*(n+m), 1);

	auto feats=new CDenseFeatures<float64_t>(data);
	SG_REF(feats);

	auto kernel=some<CGaussianKernel>();
	kernel->set_width(2.0);

	kernel->init(feats, feats);
	auto mat=kernel->get_kernel_matrix();

	// compute using within-block-permutation functor
    operation compute=shogun::internal::mmd::WithinBlockPermutation(n, EStatisticType::BIASED_FULL);
	sg_rand->set_seed(12345);
	auto result_1=compute(mat);

	compute=shogun::internal::mmd::BiasedFull(n);

	// compute a row-column permuted temporary matrix first
	// then compute a biased-full statistic on this matrix
	Map<MatrixXd> map(mat.matrix, mat.num_rows, mat.num_cols);
	PermutationMatrix<Dynamic, Dynamic> perm(mat.num_rows);
	perm.setIdentity();
	SGVector<int> perminds(perm.indices().data(), perm.indices().size(), false);
	sg_rand->set_seed(12345);
	CMath::permute(perminds);
	MatrixXd permuted = perm.transpose()*map*perm;
	SGMatrix<float64_t> permuted_km(permuted.data(), permuted.rows(), permuted.cols(), false);
	auto result_2=compute(permuted_km);

	// shuffle the features first, recompute the kernel matrix using
	// shuffled samples, then compute a biased-full statistic on this matrix
	SGVector<index_t> inds(mat.num_rows);
	std::iota(inds.vector, inds.vector+inds.vlen, 0);
	sg_rand->set_seed(12345);
	CMath::permute(inds);
	feats->add_subset(inds);
	kernel->init(feats, feats);
	mat=kernel->get_kernel_matrix();
	auto result_3=compute(mat);

	EXPECT_NEAR(result_1, result_2, 1E-15);
	EXPECT_NEAR(result_1, result_3, 1E-15);

	kernel->remove_lhs_and_rhs();
}

TEST(WithinBlockPermutation, unbiased_full)
{
	const index_t dim=2;
	const index_t n=3;
	const index_t m=4;

	using operation=std::function<float64_t(SGMatrix<float64_t>)>;

	SGMatrix<float64_t> data(dim, n+m);
	std::iota(data.matrix, data.matrix+dim*(n+m), 1);

	auto feats=new CDenseFeatures<float64_t>(data);
	SG_REF(feats);

	auto kernel=some<CGaussianKernel>();
	kernel->set_width(2.0);

	kernel->init(feats, feats);
	auto mat=kernel->get_kernel_matrix();

	// compute using within-block-permutation functor
    operation compute=shogun::internal::mmd::WithinBlockPermutation(n, EStatisticType::UNBIASED_FULL);
	sg_rand->set_seed(12345);
	auto result_1=compute(mat);

	compute=shogun::internal::mmd::UnbiasedFull(n);

	// compute a row-column permuted temporary matrix first
	// then compute unbiased-full statistic on this matrix
	Map<MatrixXd> map(mat.matrix, mat.num_rows, mat.num_cols);
	PermutationMatrix<Dynamic, Dynamic> perm(mat.num_rows);
	perm.setIdentity();
	SGVector<int> perminds(perm.indices().data(), perm.indices().size(), false);
	sg_rand->set_seed(12345);
	CMath::permute(perminds);
	MatrixXd permuted = perm.transpose()*map*perm;
	SGMatrix<float64_t> permuted_km(permuted.data(), permuted.rows(), permuted.cols(), false);
	auto result_2=compute(permuted_km);

	// shuffle the features first, recompute the kernel matrix using
	// shuffled samples, then compute unbiased-full statistic on this matrix
	SGVector<index_t> inds(mat.num_rows);
	std::iota(inds.vector, inds.vector+inds.vlen, 0);
	sg_rand->set_seed(12345);
	CMath::permute(inds);
	feats->add_subset(inds);
	kernel->init(feats, feats);
	mat=kernel->get_kernel_matrix();
	auto result_3=compute(mat);

	EXPECT_NEAR(result_1, result_2, 1E-15);
	EXPECT_NEAR(result_1, result_3, 1E-15);

	kernel->remove_lhs_and_rhs();
}

TEST(WithinBlockPermutation, unbiased_incomplete)
{
	const index_t dim=2;
	const index_t n=5;
	const index_t m=5;

	using operation=std::function<float64_t(SGMatrix<float64_t>)>;

	SGMatrix<float64_t> data(dim, n+m);
	std::iota(data.matrix, data.matrix+dim*(n+m), 1);

	auto feats=new CDenseFeatures<float64_t>(data);
	SG_REF(feats);

	auto kernel=some<CGaussianKernel>();
	kernel->set_width(2.0);

	kernel->init(feats, feats);
	auto mat=kernel->get_kernel_matrix();

	// compute using within-block-permutation functor
    operation compute=shogun::internal::mmd::WithinBlockPermutation(n, EStatisticType::UNBIASED_INCOMPLETE);
	sg_rand->set_seed(12345);
	auto result_1=compute(mat);

	compute=shogun::internal::mmd::UnbiasedIncomplete(n);

	// compute a row-column permuted temporary matrix first
	// then compute unbiased-incomplete statistic on this matrix
	Map<MatrixXd> map(mat.matrix, mat.num_rows, mat.num_cols);
	PermutationMatrix<Dynamic, Dynamic> perm(mat.num_rows);
	perm.setIdentity();
	SGVector<int> perminds(perm.indices().data(), perm.indices().size(), false);
	sg_rand->set_seed(12345);
	CMath::permute(perminds);
	MatrixXd permuted = perm.transpose()*map*perm;
	SGMatrix<float64_t> permuted_km(permuted.data(), permuted.rows(), permuted.cols(), false);
	auto result_2=compute(permuted_km);

	// shuffle the features first, recompute the kernel matrix using
	// shuffled samples, then compute uniased-incomplete statistic on this matrix
	SGVector<index_t> inds(mat.num_rows);
	std::iota(inds.vector, inds.vector+inds.vlen, 0);
	sg_rand->set_seed(12345);
	CMath::permute(inds);
	feats->add_subset(inds);
	kernel->init(feats, feats);
	mat=kernel->get_kernel_matrix();
	auto result_3=compute(mat);

	EXPECT_NEAR(result_1, result_2, 1E-15);
	EXPECT_NEAR(result_1, result_3, 1E-15);

	kernel->remove_lhs_and_rhs();
}