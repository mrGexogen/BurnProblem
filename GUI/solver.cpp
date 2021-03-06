//
// Created by tsv on 29.03.17.
//

#include "solver.hpp"

int meta_type = qRegisterMetaType<QVector<QPointF>>("QVector<QPointF>");

Solver::Solver(QObject* parent)
		: QObject(parent)
		, emit_period(1000000)
		, done(false)
		, solver(new BurnSolver)
{
}

Solver::~Solver()
{
	delete solver;
}

void
Solver::start()
{
	QThread* thread = new QThread;
	moveToThread(thread);

	connect(thread, &QThread::started, this, &Solver::work);
	connect(this, &Solver::finished, thread, &QThread::quit);
	connect(thread, &QThread::finished, thread, &QThread::deleteLater);

	done = false;

	thread->start();
}

void
Solver::work()
{
	size_t i = 0;
	qreal u_prev = 0.0;

	while (!done) {
		solver->step();

		++i;
		if (i > emit_period) {
			send();

			if (qFabs(u - u_prev) > eps_u) {
				u_prev = u;
			}
			else {
				done = true;
			}

			i = 0;
		}
		QApplication::processEvents();
	}

	send();
	moveToThread(QApplication::instance()->thread());
	emit finished();
}

void
Solver::stop()
{
	done = true;
}

void
Solver::set_emit_period(size_t period)
{
	emit_period = period;
}

void
Solver::set_eps_u(qreal eps)
{
	eps_u = eps;
}

void
Solver::send()
{
	QVector<QPointF> data;

	size_t n = solver->get_n();
	for (auto i = 0; i < n; ++i) {
		float x = solver->get_x()[i];
		float y = solver->get_y()[i];
		data.append(QPointF(x, y));
	}

	emit send_data(data);

	const QPointF& p_0 = data[0];
	const QPointF& p_1 = data[1];
	double dg_0 = (p_1.y() - p_0.y()) / (p_1.x() - p_0.x());
	u = solver->params.u(dg_0);

	emit send_u(u);
}
