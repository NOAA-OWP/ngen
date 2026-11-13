#ifndef GEOJSON_FEATUREVISITOR_H
#define GEOJSON_FEATUREVISITOR_H

namespace geojson {
    class PointFeature;
    class LineStringFeature;
    class PolygonFeature;
    class MultiPointFeature;
    class MultiLineStringFeature;
    class MultiPolygonFeature;
    class CollectionFeature;

    /**
     * An abstract class used to operate on the children of FeatureBase
     */
    class FeatureVisitor {
        public:
            /**
             * Perform an action on a PointFeature
             * 
             * @param feature A pointer to the point feature to operate upon
             */
            virtual void visit(PointFeature *feature) = 0;
            virtual void visit(LineStringFeature *feature) = 0;
            virtual void visit(PolygonFeature *feature) = 0;
            virtual void visit(MultiPointFeature *feature) = 0;
            virtual void visit(MultiLineStringFeature *feature) = 0;
            virtual void visit(MultiPolygonFeature *feature) = 0;
            virtual void visit(CollectionFeature* feature) = 0;

            virtual ~FeatureVisitor() = default;
    };
}
#endif
